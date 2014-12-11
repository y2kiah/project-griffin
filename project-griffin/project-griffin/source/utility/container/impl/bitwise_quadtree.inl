/**
 * @file	bitwise_quadtree.inl
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_BITWISE_QUADTREE_INL
#define GRIFFIN_BITWISE_QUADTREE_INL

#include <utility/container/bitwise_quadtree.h>

namespace griffin {

	//----------------------------------------------------------------------------------------
	// This function is called upon the first arrival at each node. If it returns true, the
	// node will be processed as a leaf, onLeafVisit will be called, and traversal will step
	// back to the parent. If it returns false, and the node is not a max level node (which is
	// inherently a leaf), onBranchFirstVisit will be called and traversal will progress to
	// each child node before going back to the parent. First arrival at a node occurs when
	// traversal progresses from a parent to a child node; it does not occur when traversal
	// progresses from a child back to the parent.
	//----------------------------------------------------------------------------------------
	//template <class T>
	//inline bool TBinTree<T>::onIsLeafNode(int level, const BitField<T> *index) const
	//{
	//	_ASSERTE(index);
	//	if (mOnIsLeafNodePtr) {
	//		return mOnIsLeafNodePtr(level, (T*)index);
	//	} else {
	//		return false;
	//	}
	//}

	//----------------------------------------------------------------------------------------
	// This function is called upon first visit to a branch node, as designated by the return
	// value of onIsLeafNode. The node must not be a max level node, which are inherently leaf
	// nodes. First visit occurs when traversal progresses from a parent to a child for the
	// first time; it does not occur when traversal progresses from a child back to the parent
	// node.
	//----------------------------------------------------------------------------------------
	//template <class T>
	//inline void TBinTree<T>::onBranchFirstVisit(int level, const BitField<T> *index) const
	//{
	//	_ASSERTE(index);
	//	if (mOnBranchFirstVisitPtr) {
	//		mOnBranchFirstVisitPtr(level, (T*)index);
	//	}
	//
	///*	for (int l = 0; l < level; ++l) debugPrintf("  ");
	//	debugPrintf("onBranchFirstVisit: level=%d (", level);
	//	for (int d = 0; d < mNumDimensions; ++d) {
	//		if (d != 0) debugPrintf(","); debugPrintf("%d", index[d].bits());
	//	}
	//	debugPrintf(")\n");*/
	//}

	//----------------------------------------------------------------------------------------
	//	This function is called upon visit to a leaf node, as designated by the return value
	//	of onIsLeafNode, or a max level node being an inherent leaf.
	//----------------------------------------------------------------------------------------
	//template <class T>
	//inline void TBinTree<T>::onLeafVisit(int level, const BitField<T> *index) const
	//{
	//	_ASSERTE(index);
	//	if (mOnLeafVisitPtr) {
	//		mOnLeafVisitPtr(level, (T*)index);
	//	}
	//
	///*	for (int l = 0; l < level; ++l) debugPrintf("  ");
	//	debugPrintf("onLeafVisit: level=%d (", level);
	//	for (int d = 0; d < mNumDimensions; ++d) {
	//		if (d != 0) debugPrintf(","); debugPrintf("%d", index[d].bits());
	//	}
	//	debugPrintf(")\n");*/
	//}

	//----------------------------------------------------------------------------------------
	//	This function traverses a binary tree of arbitrary depth and dimensions, where the
	//	number of depth levels is limited to the precision of T, and the number of children
	//	per node is 2^N where N is the number of dimensions. Traversal will start at minLevel
	//	and treat maxLevel nodes as leaf nodes. The location of start when minLevel > 0 is
	//	specified by the index array, where each element represents an index coordinate for
	//	one dimension, and the coordinates must be normalized to table-space for the level
	//	that is being indexed. Table-space can be calculated as the range [ 0, 1<<minLevel ).
	//	Min and max level range is [ 0, sizeof(T)*8 ]. Max level must be >= min level.
	//	Traversal order is low to high level, (parent to child, root to leaf), looping first
	//	by dimension 0, then 1, etc. So a quad-tree branch would traverse to its NW, then NE,
	//	then SW, then SE children in that order.
	//----------------------------------------------------------------------------------------
	template <typename T>
	inline void bitwise_quadtree<T>::depthFirstSearch(
			const int minLevel, const int maxLevel,
			const uint32_t x, const uint32_t y)
	{
		assert(minLevel >= 0);
		assert(maxLevel <= mMaxLevel);
		assert(maxLevel >= minLevel);
	}


	/**
	 * Calculates the depth level in the quadtree that an object with a set of tree-space
	 * coordinates will be placed
	 */
	template <class T>
	inline uint32_t bitwise_quadtree<T>::calcTreeLevel(
			const uint32_t lowX, const uint32_t highX,
			const uint32_t lowY, const uint32_t highY)
	{
		uint32_t sigBit = sSigBit;

		// XOR the position values
		uint32_t xorX = lowX ^ highX;
		uint32_t xorY = lowY ^ highY;
		
		/*uint32_t mask = sSize / 2;
		uint32_t bitCount = sSigBit;
		// this loop finds the highest set bit by ANDing with a mask
		while ((mask & highBitSet) != mask) {
			mask >>= 1;
			// negate bitCount and check if it's at deepest level already
			if (--bitCount == LEVEL_DIFF) return MAX_LEVEL;
		}*/
		
		// find highest set bit in the XOR'd values
		uint32_t highBitX = 0, highBitY = 0;
		_BitScanReverse(&highBitX, xorX);
		_BitScanReverse(&highBitY, xorY);

		uint32_t treeLevelX = sigBit - highBitX;
		uint32_t treeLevelY = sigBit - highBitY;

		// return the lower of the two tree levels
		return (treeLevelX < treeLevelY) ? treeLevelX : treeLevelY;
	}

	//----------------------------------------------------------------------------------------
	//	This function determines the tree location in array space that the object will
	//	belong to. The depth must be known so the correct index boundaries can be
	//	determined. For example, the coordinates of level 5 of the tree range from 0 - 31,
	//	or a total of 2^5 index values. Computes for only 1 axis coordinate at a time.
	//----------------------------------------------------------------------------------------
	template <class T>
	inline uint32_t bitwise_quadtree<T>::calcLocationIndex(const uint32_t pos, const int level)
	{
		// this finds the location in the array of the position that is given
		return pos >> (sNumBits - level);
	}

	//----------------------------------------------------------------------------------------
	//	This function determines the index into the array of nodes of the top left node of
	//	the children of the provided node. This is used to traverse the tree more deeply
	//	(to find visible nodes or other). The function takes a node index value obtained by
	//	the function above. It works on only one axis at a time.
	//----------------------------------------------------------------------------------------
	template <class T>
	inline uint32_t bitwise_quadtree<T>::calcChildrenPosIndex(const uint32_t locInd)
	{
		return locInd << 1;
	}

	//----------------------------------------------------------------------------------------
	//	Same as above but finds the location of the parent node.
	//----------------------------------------------------------------------------------------
	template <class T>
	inline uint32_t bitwise_quadtree<T>::calcParentPosIndex(const uint32_t locInd)
	{
		return locInd >> 1;
	}

	/**
	* Constructor
	*/
	template <typename T>
	inline bitwise_quadtree<T>::bitwise_quadtree(uint32_t maxLevel) :
		mMaxLevel{ maxLevel },
		mLevelDiff{ sSigBit - maxLevel }
	{
		assert(maxLevel <= sSigBit);
	}

}

#endif