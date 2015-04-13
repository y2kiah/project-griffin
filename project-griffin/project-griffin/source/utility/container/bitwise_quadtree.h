/**
 * @file	bitwise_quadtree.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_BITWISE_QUADTREE_H_
#define GRIFFIN_BITWISE_QUADTREE_H_

#include <cstdint>
#include <climits>

namespace griffin {

	/**
	 * @class bitwise_quadtree
	 * 32 levels (level 31 => 0-4294967295)
	 * @tparam T	
	 */
	template <typename T>
	class bitwise_quadtree {
	public:
		// Static Variables
		static const int sNumDimensions = 2;
		static const int sNumNodeChildren = 4;
		static const int sNumBits   = sizeof(uint32_t) * 8;	//!< number of bits equal to number of quadtree levels, e.g. 8
		static const int sSigBit    = sNumBits - 1;			//!< index of most significant bit, e.g. 7
		static const uint32_t sSize = UINT32_MAX;			//!< the number of nodes per axis at the highest level, 2^n, e.g. 4294967295

		// Public Functions
		int calcTreeLevel(const uint32_t lowX, const uint32_t highX,
						  const uint32_t lowY, const uint32_t highY);
		uint32_t getLocalNodeIndex(const uint32_t pos, const int level);
		uint32_t calcChildrenPosIndex(const uint32_t locInd);
		uint32_t calcParentPosIndex(const uint32_t locInd);

		void     depthFirstSearch(const int minLevel, const int maxLevel,
								  const uint32_t x, const uint32_t y);

		explicit bitwise_quadtree(int maxLevel = sSigBit);

	private:
		// Private Variables
		int mMaxLevel;   //!< the maximum depth level of the quadtree, cannot be higher than sSigBit
		int mLevelDiff;  //!< = sSigBit - mMaxLevel
	};

}

#include "impl/bitwise_quadtree-inl.h"

#endif