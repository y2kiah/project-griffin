#include "../ComponentStoreSerialization.h"
#include "../components.h"
#include "../ComponentStore.h"
#include <vector>
#include <tuple>
#include <SDL_log.h>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <utility/prettyprint.h>
#include <fstream>
#include <sstream>
#include <memory>
#include <utility/profile/Profile.h>

#include <application/Timer.h>
#include <utility/concurrency.h>

using namespace griffin::entity;

const size_t numTestComponents = 100000;
ComponentStore<Person> personStore(numTestComponents);
std::vector<ComponentId> componentIds;
std::vector<std::unique_ptr<Person>> personHeap;
griffin::Timer timer;

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
	personHeap.reserve(numTestComponents);

	// profile these and compare
	
	timer.start();
	// store 100,000 components in a ComponentStore
	componentIds = personStore.createComponents(numTestComponents, {{{0,0,0,0}}});
	timer.stop();
	SDL_Log("**********\ncreate components in store\ntime = %f ms\ncounts = %lld\n\n", timer.millisPassed(), timer.countsPassed());

	timer.start();
	// store 100,000 components on the heap
	for (int i = 0; i < numTestComponents; ++i) {
		personHeap.push_back(std::unique_ptr<Person>(new Person{0}));
	}
	timer.stop();
	SDL_Log("**********\ncreate components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisPassed(), timer.countsPassed());
}

void profileTestComponents() {
	int age = 0;

	personStore.getComponent(componentIds.back()).age = 10;
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personStore.getComponent(componentIds[i]).age;
	}
	timer.stop();
	SDL_Log("**********\nloop components with external id\ntime = %f ms\ncounts = %lld\n\n", timer.millisPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age); // use age so it doesn't compile away in release build test

	auto cmp = personStore.getComponents().getItems();
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += cmp[i].component.age;
	}
	timer.stop();
	SDL_Log("**********\nloop components inner array\ntime = %f ms\ncounts = %lld\n\n", timer.millisPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age);

	personHeap.back()->age = 5;
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personHeap[i]->age;
	}
	timer.stop();
	SDL_Log("**********\nloop components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisPassed(), timer.countsPassed());

	SDL_Log("age=%d\n", age);
}

void griffin::entity::test_reflection() {
	// test concurrency system
	griffin::task<int> tsk;
	auto& fut1 = tsk.run([]{
		std::this_thread::sleep_for(std::chrono::seconds(30));
		SDL_Log("task 1 step 1");
		return 1;
	});
	auto& fut2 = tsk.then([]{
		SDL_Log("task 1 step 2");
		return 2.0f;
	});

	griffin::task<void> tsk2;
	tsk2.run([](std::shared_future<int> tsk1Fut, float f){
		SDL_Log("task 2 step 1, see value %d, %.1f", tsk1Fut.get(), f);
	}, fut1.get_future(), 3.0f) // to support non-lambda functions, this syntax can be used to pass a parameter instead of capturing it
	.then([]{
		SDL_Log("task 2 step 2");
	})
	.then([]{
		SDL_Log("task 2 step 3 OpenGL");
	}, ThreadAffinity::Thread_OpenGL_Render);

	fut2.then([fut2]{ // when using lambda you can capture a previous step's task/future and get its value
		SDL_Log("task 1 step 3, see value %.1f", fut2.get());
	})
	.then([]{
		SDL_Log("task 1 step 4");
	});

	// test component store
	addTestComponents();
	//profileTestComponents();

	SDL_Log(personStore.to_string().c_str());
	Person& person = personStore.getComponent(componentIds[0]);
	
	auto& personProps = Person::Reflection::getProperties();
	auto vals = Person::Reflection::getAllValues(person);

	SDL_Log("%s:\n", Person::Reflection::getClassType().c_str());
	for (const auto &f : personProps) {
		SDL_Log("%s : %s\n", f.name.c_str(), f.description.c_str());
	}
	
	int   a = std::get<Person::Reflection::age>(vals);
	float b = std::get<Person::Reflection::speed>(vals);
	char* c = std::get<Person::Reflection::name>(vals);
	SDL_Log("person values: %d, %f, %s\n", a, b, c);
	SDL_Log("age description: %s\n", personProps[Person::Reflection::FieldToEnum("age")].description.c_str());
	std::get<Person::Reflection::age>(vals) = 30;
	std::get<Person::Reflection::speed>(vals) = 4.0f;
	SDL_Log("age after set = %d\n", person.age); // should now be 30
	SDL_Log("speed after set = %1.1f\n", person.speed); // should now be 4

	ComponentMask bit_test;
	bit_test.set(ComponentType::SceneNode_T, true);
	ComponentMask bit_and(0xFFFFFFFF);
	ComponentMask bit_res = bit_test | bit_and;
	SDL_Log("bit_test = %s\n", bit_test.to_string().c_str());
	SDL_Log("bit_and  = %s\n", bit_and.to_string().c_str());
	SDL_Log("bit_res  = %s\n", bit_res.to_string().c_str());

	SDL_Log("Person is POD = %d\n", std::is_pod<Person>::value);
	SDL_Log("Something is POD = %d\n", std::is_pod<Something>::value);
	SDL_Log("ComponentId is POD = %d\n", std::is_pod<ComponentId>::value);

	Something s = { 1, 2, "hi" };
	auto s0 = getMemberVal(s, SomethingPred0{});
	SDL_Log("s val 0 through template = %d\n", s0);

	// output person properties
	SDL_Log("%s {\n", Person::Reflection::getClassType().c_str());
	for (auto p : Person::Reflection::getProperties()) {
		SDL_Log("  %s (%d): %s, isArray=%d, isTriviallyCopyable=%d\n", p.name.c_str(), p.size, p.description.c_str(), p.isArray, p.isTriviallyCopyable);
	}
	SDL_Log("}\n\n");

	// testing tuple for_each
	using namespace boost::fusion;
	std::ostringstream oss;
	oss << vals;
	SDL_Log("\n\n%s\n\n", oss.str().c_str());

	// testing ComponentId comparisons
	// we want the ComponentType portion to sort at a higher priority than the index or generation,
	vector<ComponentId> sortIds = {
			{{{0, 0, ComponentType::Person_T, 0}}},
			{{{0, 0, ComponentType::Person_T, 0}}},
			{{{0xFFFFFFFF, 0xFFFF, ComponentType::Person_T, 0}}},
			{{{50, 1, ComponentType::Person_T, 1}}},
			{{{5, 1, ComponentType::Person_T, 0}}},
			{{{5, 1, ComponentType::SceneNode_T, 0}}},
			{{{5, 3, ComponentType::Person_T, 0}}}
		};
	std::sort(sortIds.begin(), sortIds.end());
	oss.clear();
	oss << sortIds;
	SDL_Log(oss.str().c_str());

	// test ComponentStore serialization
	std::ofstream ofs;
	ofs.open("component_serialize.hex", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	timer.start();
	serialize(ofs, personStore);
	timer.stop();
	SDL_Log("**********\nsave components to file\ntime = %f ms\ncounts = %lld\n", timer.millisPassed(), timer.countsPassed());
	ofs.close();
	SDL_Log("personStore saved:\n%s\n\n", personStore.to_string().c_str());

	ComponentStore<Person> personStoreReadTest(numTestComponents);
	std::ifstream ifs;
	ifs.open("component_serialize.hex", std::ifstream::in | std::ifstream::binary);
	ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	timer.start();
	deserialize(ifs, personStoreReadTest);
	timer.stop();
	SDL_Log("**********\nread components from file\ntime = %f ms\ncounts = %lld\n", timer.millisPassed(), timer.countsPassed());
	ifs.close();
	SDL_Log("personStore read:\n%s\n\n", personStoreReadTest.to_string().c_str());
}
