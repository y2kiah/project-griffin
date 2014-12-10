/* ----==== QUADTREE.H ====---- */

#pragma once

//#include <boost/tr1/memory.hpp>
#include <hash_map>
#include "../Utility/Typedefs.h"
#include "TBinTree.h"

//using std::tr1::shared_ptr;
using stdext::hash_map;


///// DEFINES /////

#define	Quadtree8	TQuadtree<uchar>
#define	Quadtree16	TQuadtree<ushort>
#define	Quadtree32	TQuadtree<uint>
#define	Quadtree64	TQuadtree<ulong>

///// STRUCTURES /////

template <class T> class TQuadtreeLevel;
template <class T> class TQuadtree;

//=============================================================================
// Interface to a quadtree node structure, it is expected that casting will be
// done instead of polymorphism, because it will be used in a context where the
// derived type is known.
//=============================================================================
class IQuadtreeNode abstract {
	private:
	public:
		explicit IQuadtreeNode() {}
		virtual ~IQuadtreeNode() {}
};

//=============================================================================
//	Contains the quadtree level's nodes
//=============================================================================
template <class T>
class TQuadtreeLevel {
	friend class TQuadtree<T>;
	private:
		int								mLevel;		// the level number
		hash_map<T, IQuadtreeNode*>		mNodes;		// an array of quadtree nodes at this level

	public:
		explicit TQuadtreeLevel() : mLevel(0) {}
		explicit TQuadtreeLevel(const T level) : mLevel(level) {}
		~TQuadtreeLevel() {}
};

//=============================================================================
//=============================================================================
template <class T>
class TQuadtree {
	private:
		///// VARIABLES /////
		TQuadtreeLevel<T> *			mLevels;	// dynamic array of level pointers in the quadtree
		int							mMaxLevel;	// the maximum depth level of the quadtree, cannot be higher than SIG_BIT
		int							mLevelDiff; // = SIG_BIT - mMaxLevel

		TBinTree<T>					mQuadTree;

	public:
		static const int NUM_BITS;	// number of bits for SIZE, 2^8 = 256, 2^9 = 512, etc.
		static const int SIG_BIT;	// index of most significant bit for SIZE, = NUM_BITS - 1
		static const int SIZE;		// the number of nodes per axis, MUST be 2^n
		static const int MSK_START;	// must be SIZE >> 1 (or SIZE / 2)
		
		///// FUNCTIONS /////

		// Mutators
		inline void		setOnIsLeafNode(boost::function<bool (int level, const T *index)> func);
		inline void		setOnBranchFirstVisit(boost::function<void (int level, const T *index)> func);
		inline void		setOnLeafVisit(boost::function<void (int level, const T *index)> func);
		bool			setMaxLevel(int maxLevel) { mMaxLevel = maxLevel; }

		// Quadtree functions
		inline T		calcTreeLevel(const T lowPos, const T highPos);
		inline T		calcLocationIndex(const T pos, const T level);
		inline T		calcChildrenPosIndex(const T locInd);
		inline T		calcParentPosIndex(const T locInd);
		inline void		traverse(int minLevel, int maxLevel, T x, T y);

		// Constructor / Destructor
		explicit TQuadtree(int maxLevel);
		~TQuadtree();
};

// class TQuadtree

///// STATIC VARIABLES /////

template <class T> const int TQuadtree<T>::NUM_BITS		= sizeof(T) * 8;				// e.g. 8
template <class T> const int TQuadtree<T>::SIG_BIT		= TQuadtree<T>::NUM_BITS - 1;	// e.g. 7
template <class T> const int TQuadtree<T>::SIZE			= 1 << TQuadtree<T>::NUM_BITS;	// e.g. 256
template <class T> const int TQuadtree<T>::MSK_START	= TQuadtree<T>::SIZE >> 1;		// e.g. 128

///// INLINE FUNCTIONS /////

template <class T>
void TQuadtree<T>::setOnIsLeafNode(boost::function<bool (int level, const T *index)> func)
{
	mQuadTree.mOnIsLeafNodePtr = func;
}

template <class T>
void TQuadtree<T>::setOnBranchFirstVisit(boost::function<void (int level, const T *index)> func)
{
	mQuadTree.mOnBranchFirstVisitPtr = func;
}

template <class T>
void TQuadtree<T>::setOnLeafVisit(boost::function<void (int level, const T *index)> func)
{
	mQuadTree.mOnLeafVisitPtr = func;
}

//----------------------------------------------------------------------------------------
//	This function calculates the depth level into a quadtree that an object from
//	coordinates lowPos to highPos (in array space 0-255) will be placed. Note: this
//	only checks one axis, so both axes will need to be checked and the correct level
//	is the smaller of the two
//----------------------------------------------------------------------------------------
template <class T>
inline T TQuadtree<T>::calcTreeLevel(const T lowPos, const T highPos)
{
	// XOR the position values, then find highest set bit
	T highBitSet = lowPos ^ highPos;
	T mask = MSK_START;
	T bitCount = SIG_BIT;

	// this loop finds the highest set bit by ANDing with a mask
	while ((mask & highBitSet) != mask) {
		mask >>= 1;
		// negate bitCount and check if it's at deepest level already
		if (--bitCount == LEVEL_DIFF) return MAX_LEVEL;
	}

	// return the tree depth level
	return SIG_BIT - bitCount;
}

//----------------------------------------------------------------------------------------
//	This function determines the tree location in array space that the object will
//	belong to. The depth must be known so the correct index boundaries can be
//	determined. For example, the coordinates of level 5 of the tree range from 0 - 31,
//	or a total of 2^5 index values. Computes for only 1 axis coordinate at a time.
//----------------------------------------------------------------------------------------
template <class T>
inline T TQuadtree<T>::calcLocationIndex(const T pos, const T level)
{
	// this finds the location in the array of the position that is given
	return pos >> (NUM_BITS - level);
}

//----------------------------------------------------------------------------------------
//	This function determines the index into the array of nodes of the top left node of
//	the children of the provided node. This is used to traverse the tree more deeply
//	(to find visible nodes or other). The function takes a node index value obtained by
//	the function above. It works on only one axis at a time.
//----------------------------------------------------------------------------------------
template <class T>
inline T TQuadtree<T>::calcChildrenPosIndex(const T locInd)
{
	return locInd << 1;
}

//----------------------------------------------------------------------------------------
//	Same as above but finds the location of the parent node.
//----------------------------------------------------------------------------------------
template <class T>
inline T TQuadtree<T>::calcParentPosIndex(const T locInd)
{
	return locInd >> 1;
}

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
template <class T>
inline void	TQuadtree<T>::traverse(int minLevel, int maxLevel, T x, T y)
{
	_ASSERTE(maxLevel <= mMaxLevel);
	T index[2] = {x,y};

	mQuadTree.traverse(minLevel, maxLevel, index);
}

// Constructor
template <class T>
TQuadtree<T>::TQuadtree(int maxLevel) : mQuadTree(2)
{
	mMaxLevel = (maxLevel > SIG_BIT) ? SIG_BIT : maxLevel;
	mLevelDiff = SIG_BIT - mMaxLevel;
	mLevels = new TQuadtreeLevel<T>[mMaxLevel];
	for (int l = 0; l < mMaxLevel; ++l) mLevels[l].mLevel = l;
}

// Destructor
template <class T>
TQuadtree<T>::~TQuadtree()
{
	delete [] mLevels;
}