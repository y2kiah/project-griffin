#include "Test.h"
#include <cstdint>
#include <sstream>
#include <utility/Logger.h>
#include <utility/container/bitwise_quadtree.h>


using namespace griffin;

void testBitwiseQuadtree(Logger& log) {
	using namespace std;

	ostringstream oss;

	uint32_t splitLevel0 = UINT32_MAX / 2;
	uint32_t splitLevel1 = UINT32_MAX / 4;
	uint32_t splitLevel2 = UINT32_MAX / 8;

	griffin::bitwise_quadtree<int> quadtree;
	int result0 = quadtree.calcTreeLevel(splitLevel0 - 1, splitLevel0 + 1, splitLevel0 - 1, splitLevel0 + 1);
	int result1 = quadtree.calcTreeLevel(splitLevel1 - 1, splitLevel1 + 1, splitLevel1 - 1, splitLevel1 + 1);
	int result2 = quadtree.calcTreeLevel(splitLevel2 - 1, splitLevel2 + 1, splitLevel2 - 1, splitLevel2 + 1);
	int result31 = quadtree.calcTreeLevel(0, 0, 0, 0);
	int result30 = quadtree.calcTreeLevel(0, 0, 0, 2);

	for (int x = 0; x < 34; ++x) {
		oss << x << "," << x + 1 << " = " << quadtree.calcTreeLevel(0, 0, x, x + 1) << endl;
	}

	assert(0 == result0 && L"Split Level 0, X and Y match");
	assert(1 == result1 && L"Split Level 1, X and Y match");
	assert(2 == result2 && L"Split Level 2, X and Y match");
	assert(31 == result31 && L"Split Level 31, X and Y match");
	assert(30 == result30 && L"Split Level 30, X and Y don't match");

	log.info(Logger::Category_Test, oss.str().c_str());
}
