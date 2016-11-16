#include "Test.h"
//#include "../ComponentStoreSerialization.h"
#include <entity/components.h>
#include <entity/ComponentStore.h>
#include <vector>
#include <tuple>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
//#include <utility/prettyprint.h>
#include <fstream>
#include <sstream>
#include <memory>
#include <utility/profile/Profile.h>
#include <utility/Logger.h>
#include <scene/SceneGraph.h>
#include <application/Timer.h>
#include <utility/Logger.h>


using namespace griffin::entity;
using namespace griffin;

const size_t numTestComponents = 100000;
ComponentStore<scene::SceneNode> sceneNodeStore(numTestComponents);
std::vector<ComponentId> componentIds;
std::vector<std::unique_ptr<scene::SceneNode>> sceneNodeHeap;
static Timer timer;

struct Something {
	int x, y;
	char z[16];
};

struct SomethingPred0 {
	int Something::* operator()() const {
		return &Something::x;
	}
};

template<typename T, typename F>
auto getMemberVal(const T& t, F f) -> decltype(t.*f()) {
	return t.*f();
}

// example cast objects


/**
* The following two functions demonstrate the performance difference between iterating through
* components allocated contiguously vs allocated individually on the heap and referenced through
* a unique_ptr
*/
void addTestComponents() {
	componentIds.reserve(numTestComponents);
	sceneNodeHeap.reserve(numTestComponents);

	// profile these and compare

	timer.start();
	// store 100,000 components in a ComponentStore
	componentIds = sceneNodeStore.createComponents(numTestComponents, EntityId{});
	timer.stop();
	logger.test("**********\ncreate components in store\ntime = %f ms\ncounts = %lld\n\n", timer.getMillisPassed(), timer.getCountsPassed());

	timer.start();
	// store 100,000 components on the heap
	for (int i = 0; i < numTestComponents; ++i) {
		sceneNodeHeap.push_back(std::unique_ptr<scene::SceneNode>(new scene::SceneNode{ 0 }));
	}
	timer.stop();
	logger.test("**********\ncreate components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.getMillisPassed(), timer.getCountsPassed());
}

void profileTestComponents() {
	glm::dvec3 trans = { 0, 0, 0 };

	sceneNodeStore.getComponent(componentIds.back()).translationLocal = { 10, 0, 0 };
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		trans += sceneNodeStore.getComponent(componentIds[i]).translationLocal;
	}
	timer.stop();
	logger.test("**********\nloop components with external id\ntime = %f ms\ncounts = %lld\n\n", timer.getMillisPassed(), timer.getCountsPassed());
	logger.test("trans={%d,%d,%d}\n", trans.x, trans.y, trans.z); // use trans so it doesn't compile away in release build test

	auto cmp = sceneNodeStore.getComponents().getItems();
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		trans += cmp[i].component.translationLocal;
	}
	timer.stop();
	logger.test("**********\nloop components inner array\ntime = %f ms\ncounts = %lld\n\n", timer.getMillisPassed(), timer.getCountsPassed());
	logger.test("trans={%d,%d,%d}\n", trans.x, trans.y, trans.z);

	sceneNodeHeap.back()->translationLocal = { 5, 0, 0 };
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		trans += sceneNodeHeap[i]->translationLocal;
	}
	timer.stop();
	logger.test("**********\nloop components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.getMillisPassed(), timer.getCountsPassed());

	logger.test("trans={%d,%d,%d}\n", trans.x, trans.y, trans.z);
}

void testReflection() {
	//////////
	// test component store
	addTestComponents();
	profileTestComponents();

	logger.test(sceneNodeStore.to_string().c_str());
	scene::SceneNode& node = sceneNodeStore.getComponent(componentIds[0]);

	auto& nodeProps = scene::SceneNode::Reflection::getProperties();
	auto vals = scene::SceneNode::Reflection::getAllValues(node);

	logger.test("%s:\n", scene::SceneNode::Reflection::getClassType().c_str());
	for (const auto &f : nodeProps) {
		logger.test("%s : %s\n", f.name.c_str(), f.description.c_str());
	}

	int a = std::get<scene::SceneNode::Reflection::numChildren>(vals);
	uint8_t b = std::get<scene::SceneNode::Reflection::positionDirty>(vals);
	uint8_t c = std::get<scene::SceneNode::Reflection::orientationDirty>(vals);

	logger.test("scene node values: %d, %d, %d\n", a, b, c);
	logger.test("numChildren description: %s\n", nodeProps[scene::SceneNode::Reflection::FieldToEnum("numChildren")].description.c_str());
	std::get<scene::SceneNode::Reflection::positionDirty>(vals) = 1;
	std::get<scene::SceneNode::Reflection::orientationDirty>(vals) = 1;
	logger.test("position dirty = %d\n", node.positionDirty); // should now be 1
	logger.test("orientation dirty = %d\n", node.orientationDirty); // should now be 1

	ComponentMask bit_test;
	bit_test.set(ComponentType::SceneNode_T, true);
	ComponentMask bit_and(0xFFFFFFFF);
	ComponentMask bit_res = bit_test | bit_and;
	logger.test("bit_test = %s\n", bit_test.to_string().c_str());
	logger.test("bit_and  = %s\n", bit_and.to_string().c_str());
	logger.test("bit_res  = %s\n", bit_res.to_string().c_str());

	logger.test("SceneNode is POD = %d\n", std::is_pod<scene::SceneNode>::value);
	logger.test("Something is POD = %d\n", std::is_pod<Something>::value);
	logger.test("ComponentId is POD = %d\n", std::is_pod<ComponentId>::value);

	Something s = { 1, 2, "hi" };
	auto s0 = getMemberVal(s, SomethingPred0{});
	logger.test("s val 0 through template = %d\n", s0);

	// output scene node properties
	logger.test("%s {\n", scene::SceneNode::Reflection::getClassType().c_str());
	for (auto p : scene::SceneNode::Reflection::getProperties()) {
		logger.test("  %s (%d): %s, isArray=%d, isTriviallyCopyable=%d\n", p.name.c_str(), p.size, p.description.c_str(), p.isArray, p.isTriviallyCopyable);
	}
	logger.test("}\n\n");

	// testing tuple for_each
	/*	using namespace boost::fusion;
	std::ostringstream oss;
	oss << vals;
	logger.test("\n\n%s\n\n", oss.str().c_str());

	// testing ComponentId comparisons
	// we want the ComponentType portion to sort at a higher priority than the index or generation,
	vector<ComponentId> sortIds = {
	{{{0, 0, ComponentType::SceneNode_T, 0}}},
	{{{0, 0, ComponentType::SceneNode_T, 0}}},
	{{{0xFFFFFFFF, 0xFFFF, ComponentType::SceneNode_T, 0}}},
	{{{50, 1, ComponentType::SceneNode_T, 1}}},
	{{{5, 1, ComponentType::SceneNode_T, 0}}},
	{{{5, 1, ComponentType::SceneNode_T, 0}}},
	{{{5, 3, ComponentType::SceneNode_T, 0}}}
	};
	std::sort(sortIds.begin(), sortIds.end());
	oss.clear();
	oss << sortIds;
	logger.test(oss.str().c_str());

	// test ComponentStore serialization
	std::ofstream ofs;
	ofs.open("component_serialize.hex", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	timer.start();
	serialize(ofs, sceneNodeStore);
	timer.stop();
	logger.test("**********\nsave components to file\ntime = %f ms\ncounts = %lld\n", timer.getMillisPassed(), timer.getCountsPassed());
	ofs.close();
	logger.test("sceneNodeStore saved:\n%s\n\n", sceneNodeStore.to_string().c_str());

	ComponentStore<scene::SceneNode> sceneNodeStoreReadTest(numTestComponents);
	std::ifstream ifs;
	ifs.open("component_serialize.hex", std::ifstream::in | std::ifstream::binary);
	ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	timer.start();
	deserialize(ifs, sceneNodeStoreReadTest);
	timer.stop();
	logger.test("**********\nread components from file\ntime = %f ms\ncounts = %lld\n", timer.getMillisPassed(), timer.getCountsPassed());
	ifs.close();
	logger.test("sceneNodeStore read:\n%s\n\n", sceneNodeStoreReadTest.to_string().c_str());
	*/
}
