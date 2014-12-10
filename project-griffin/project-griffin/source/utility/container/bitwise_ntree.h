/**
 * @file	bitwise_quadtree.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_BITWISE_QUADTREE_H
#define GRIFFIN_BITWISE_QUADTREE_H

#include <cstdint>
#include <bitset>

namespace griffin {

	/**
	 * @class bitwise_ntree
	 * This class encapsulates generic non-recursive traversal algorithms for a tree of
	 * N_dimensions, where 1 << N_dimensions is the power-of-two number of child nodes for each tree
	 * node, (e.g. 1 = binary tree, 2 = quad-tree, 3 = oct-tree, etc.).
	 * The template type T determines the number of bits available at the maximum tree level, which
	 * is the total number of tree nodes at that level (per dimension).
	 * @tparam T	type determines bitwise precision, max tree level, total number of nodes
	 * @tparam N_dimensions		1 << N_dimensions is the number of children each node contains
	 */
	template <typename T, int N_dimensions>
	class bitwise_ntree {
	public:
		// Variables
		static const int nodesPerLevel = (1 << N_dimensions);
		static const int numDimensions = N_dimensions;

		std::bitset<sizeof(T)> currentIndex[N_dimensions];	//!< array of coordinates used during traversal

		// Function pointers
		//function<bool (int level, const T *index)>	mOnIsLeafNodePtr;
		//function<void (int level, const T *index)>	mOnBranchFirstVisitPtr;
		//function<void (int level, const T *index)>	mOnLeafVisitPtr;

		// Functions
		//bool onIsLeafNode(int level, const BitField<T> *index) const;
		//void onBranchFirstVisit(int level, const BitField<T> *index) const;
		//void onLeafVisit(int level, const BitField<T> *index) const;

		void traverse(int minLevel, int maxLevel, const T *index);

		explicit bitwise_ntree();
	};

	typedef bitwise_ntree<uint8_t,  1>	bitwise_bintree8;	//  8 levels (0-255)
	typedef bitwise_ntree<uint16_t, 1>	bitwise_bintree16;	// 16 levels (0-65535)
	typedef bitwise_ntree<uint32_t, 1>	bitwise_bintree32;	// 32 levels (0-4294967295)
	typedef bitwise_ntree<uint64_t, 1>	bitwise_bintree64;	// 64 levels (0-18446744073709551615)

	typedef bitwise_ntree<uint8_t,  2>	bitwise_quadtree8;
	typedef bitwise_ntree<uint16_t, 2>	bitwise_quadtree16;
	typedef bitwise_ntree<uint32_t, 2>	bitwise_quadtree32;
	typedef bitwise_ntree<uint64_t, 2>	bitwise_quadtree64;

	typedef bitwise_ntree<uint8_t,  3>	bitwise_octtree8;
	typedef bitwise_ntree<uint16_t, 3>	bitwise_octtree16;
	typedef bitwise_ntree<uint32_t, 3>	bitwise_octtree32;
	typedef bitwise_ntree<uint64_t, 3>	bitwise_octtree64;
}

#include "impl/bitwise_ntree.inl"

#endif