#include "Test.h"
#include <cstdint>
#include <sstream>
#include <utility/Logger.h>
#include <utility/container/bitwise_quadtree.h>
#include <utility/container/handle_map.h>
#include <application/Timer.h>
#include <algorithm>
#include <array>


using namespace griffin;

static Timer timer;


void testBitwiseQuadtree()
{
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

	logger.info(Logger::Category_Test, oss.str().c_str());
}


void testHandleMap()
{
	struct Test { int v1, v2, v3, v4; };
	const int N = 100000;
	int total = 0;

	// create a handle_map with itemId = 0 and a capacity of 100,000
	handle_map<Test> testMap(0, N);
	std::vector<Id_T> testHandles;
	std::vector<std::unique_ptr<Test>> testHeap;
	testHandles.reserve(N);
	testHeap.reserve(N);

	// test defragment
	const int M = 10;
	std::array<int, M> vals{};
	std::array<Id_T, M> ids{};

	for (int i = 0; i < M; ++i) { vals[i] = i + 1; }
	std::random_shuffle(vals.begin(), vals.end());
	for (int i = 0; i < M; ++i) { ids[i] = testMap.emplace(vals[i], 0, 0, 0); }

	logger.test("before defrag:");
	for (auto& t : testMap.getItems()) {
		logger.test("%d", t.v1);
	}

	int swaps1 = testMap.defragment([](const Test& a, const Test& b) { return a.v1 > b.v1; }, M);
	int swaps2 = testMap.defragment([](const Test& a, const Test& b) { return a.v1 > b.v1; }, M);
	int swaps3 = testMap.defragment([](const Test& a, const Test& b) { return a.v1 > b.v1; }, M);
	int swaps4 = testMap.defragment([](const Test& a, const Test& b) { return a.v1 > b.v1; }, M);
	int swaps5 = testMap.defragment([](const Test& a, const Test& b) { return a.v1 > b.v1; });

	logger.test("after defrag: swaps = %d, %d, %d, %d, %d", swaps1, swaps2, swaps3, swaps4, swaps5);
	for (auto& t : testMap.getItems()) {
		logger.test("%d", t.v1);
	}

	logger.test("after defrag, by handle:");
	for (int i = 0; i < M; ++i) {
		logger.test("%d", testMap[ids[i]].v1);
	}

	testMap.clear();


	// fill up the capacity with zero-initialized objects
	timer.start();
	testHandles = testMap.emplaceItems(N, 1, 1, 1, 1);
	timer.stop();
	logger.test("handle_map: create %d items, time = %f ms\ncounts = %lld\n", N, timer.getMillisPassed(), timer.getCountsPassed());
	
	// create items on the heap
	timer.start();
	for (int i = 0; i < N; ++i) {
		testHeap.emplace_back(new Test{ 1, 1, 1, 1 });
	}
	timer.stop();
	logger.test("unique_ptr: create %d items, time = %f ms\ncounts = %lld\n", N, timer.getMillisPassed(), timer.getCountsPassed());


	auto& items = testMap.getItems();
	// vector iteration
	timer.start();
	for (int i = 0; i < N; ++i) {
		auto& t = items[i];
		total += t.v1 + t.v2 + t.v3 + t.v4;
	}
	timer.stop();
	logger.test("handle_map: iteration over dense set, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// heap iteration
	total = 0;
	timer.start();
	for (int p = 0; p < N; ++p) {
		auto& t = *testHeap[p];
		total += t.v1 + t.v2 + t.v3 + t.v4;
	}
	timer.stop();
	logger.test("unique_ptr: iteration, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// handle iteration
	total = 0;
	timer.start();
	for (int h = 0; h < N; ++h) {
		auto& t = testMap[testHandles[h]];
		total += t.v1 + t.v2 + t.v3 + t.v4;
	}
	timer.stop();
	logger.test("handle_map: iteration by handle, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());


	// remove all items, destroying the sparse set
	timer.start();
	testMap.reset();
	timer.stop();
	logger.test("handle_map: clear %d items, time = %f ms\ncounts = %lld\n\n", N, timer.getMillisPassed(), timer.getCountsPassed());

	// remove all items from the heap
	timer.start();
	testHeap.clear();
	timer.stop();
	logger.test("unique_ptr: clear %d items, time = %f ms\ncounts = %lld\n\n", N, timer.getMillisPassed(), timer.getCountsPassed());
}