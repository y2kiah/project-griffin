#include "Test.h"
#include <cstdint>
#include <sstream>
#include <utility/Logger.h>
#include <utility/container/bitwise_quadtree.h>
#include <utility/container/handle_map.h>
#include <application/Timer.h>
#include <algorithm>
#include <array>
#include <unordered_map>


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
	struct Test { int val, sort; };
	const int N = 100000;
	int total = 0;

	// create a handle_map with itemTypeId of 0 and a capacity of 100,000
	handle_map<Test> testMap(0, N);
	std::vector<Id_T> testHandles;
	std::vector<std::unique_ptr<Test>> testHeap;
	std::unordered_map<uint64_t, Test> testHashMap;
	testHandles.reserve(N);
	testHeap.reserve(N);
	testHashMap.reserve(N);
	auto compareItems = [](const Test& a, const Test& b) { return a.sort > b.sort; };

	// fill up the capacity with zero-initialized objects
	timer.start();
	for (int i = 0; i < N; ++i) {
		testHandles.push_back(testMap.emplace(1, i));
	}
	timer.stop();
	logger.test("handle_map: create %d items, time = %f ms\ncounts = %lld\n", N, timer.getMillisPassed(), timer.getCountsPassed());
	
	// create items on the heap
	timer.start();
	for (int i = 0; i < N; ++i) {
		testHeap.emplace_back(new Test{ 1, i });
	}
	timer.stop();
	logger.test("unique_ptr: create %d items, time = %f ms\ncounts = %lld\n", N, timer.getMillisPassed(), timer.getCountsPassed());

	// create items in hash map
	timer.start();
	for (int i = 0; i < N; ++i) {
		testHashMap.emplace(testHandles[i].value, Test{ 1, i });
	}
	timer.stop();
	logger.test("unordered_map: create %d items, time = %f ms\ncounts = %lld\n", N, timer.getMillisPassed(), timer.getCountsPassed());


	auto& items = testMap.getItems();
	// vector iteration
	timer.start();
	for (int i = 0; i < N; ++i) {
		auto& t = items[i];
		total += t.val;
	}
	timer.stop();
	logger.test("handle_map: iteration over dense set, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// heap iteration
	total = 0;
	timer.start();
	for (int p = 0; p < N; ++p) {
		auto& t = *testHeap[p];
		total += t.val;
	}
	timer.stop();
	logger.test("unique_ptr: iteration, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// hash map iterator
	total = 0;
	timer.start();
	for (auto& test : testHashMap) {
		total += test.second.val;
	}
	timer.stop();
	logger.test("unordered_map: iteration by iterator, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// handle iteration
	total = 0;
	timer.start();
	for (int h = 0; h < N; ++h) {
		auto& t = testMap[testHandles[h]];
		total += t.val;
	}
	timer.stop();
	logger.test("handle_map: iteration by handle, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());

	// hash map iteration by lookup
	total = 0;
	timer.start();
	for (int h = 0; h < N; ++h) {
		auto& t = testHashMap[testHandles[h].value];
		total += t.val;
	}
	timer.stop();
	logger.test("unordered_map: iteration by lookup, total = %d, time = %f ms\ncounts = %lld\n\n", total, timer.getMillisPassed(), timer.getCountsPassed());


	// defrag
	std::random_shuffle(testMap.begin(), testMap.end());
	timer.start();
	size_t swaps = testMap.defragment(compareItems);
	timer.stop();
	logger.test("handle_map: defragment swaps = %llu, total = %d, time = %f ms\ncounts = %lld\n\n", swaps, total, timer.getMillisPassed(), timer.getCountsPassed());


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

	// remove all items from the hash map
	timer.start();
	testHashMap.clear();
	timer.stop();
	logger.test("unordered_map: clear %d items, time = %f ms\ncounts = %lld\n\n", N, timer.getMillisPassed(), timer.getCountsPassed());


	logger.test("test_map capacity = %d", testMap.capacity());
}