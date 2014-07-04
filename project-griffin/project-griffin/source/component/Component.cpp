#include "components.h"
#include <tuple>
#include <SDL_log.h>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <utility/prettyprint.h>
#include <fstream>
#include <memory>

#include <application/Timer.h>

const size_t numTestComponents = 100000;
auto personStore = ComponentStore<Person>(numTestComponents);
std::vector<ComponentId> componentIds;
std::vector<std::unique_ptr<Person>> personHeap;
Timer timer;

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

// test tuple iteration
struct print {
	template<typename T>
	void operator()(const vector<T>& v) const {
		std::cout << "[ ";
		for (auto p : v) {
			std::cout << p << (p != v.back() ? ", " : "");
		}
		std::cout << " ]\n";
	}

	template<typename T>
	void operator()(const T& t) const {
		std::cout << t << std::endl;
	}
};

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
	for (int i = 0; i < numTestComponents; ++i) {
		componentIds.emplace_back(personStore.createComponent());
	}
	timer.stop();
	SDL_Log("**********\ncreate components in store\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());

	timer.start();
	// store 100,000 components on the heap
	for (int i = 0; i < numTestComponents; ++i) {
		personHeap.emplace_back(new Person{});
	}
	timer.stop();
	SDL_Log("**********\ncreate components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
}

void profileTestComponents() {
	int age = 0;

	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personStore.getComponent(componentIds[i]).age;
	}
	timer.stop();
	SDL_Log("**********\nloop components with external id\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age); // use age so it doesn't compile away in release build test

	const auto& cmp = personStore.getComponents();
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += cmp[i].second.age;
	}
	timer.stop();
	SDL_Log("**********\nloop components inner array\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age);

	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personHeap[i]->age;
	}
	timer.stop();
	SDL_Log("**********\nloop components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());

	SDL_Log("age=%d\n", age);
}

void test_reflection() {
	Timer::initHighPerfTimer();

	addTestComponents();
	profileTestComponents();

	//SDL_Log(personStore.to_string().c_str());
	Person& person = personStore.getComponent(componentIds[0]);
	
	auto & personProps = Person::Reflection::getProperties();
	auto vals = Person::Reflection::getAllValues(person);

	SDL_Log("%s:\n", Person::Reflection::getClassType().c_str());
	for (const auto &f : personProps) {
		SDL_Log("%s %s : %s\n", FieldTypeToString(f.type), f.name.c_str(), f.description.c_str());
	}
	int a = std::get<Person::Reflection::age>(vals);
	float b = std::get<Person::Reflection::speed>(vals);
	auto& c = std::get<Person::Reflection::name>(vals);
	SDL_Log("person values: %d, %f, %s\n", a, b, c.c_str());
	SDL_Log("age description: %s\n", personProps[Person::Reflection::FieldsToEnum("age")].description.c_str());
	std::get<Person::Reflection::age>(vals) = 30;
	SDL_Log("age after set = %d\n", person.age); // should now be 30

	ComponentMask bit_test;
	bit_test.set(ComponentType::Position_T, true);
	ComponentMask bit_and(0xFFFFFFFF);
	ComponentMask bit_res = bit_test | bit_and;
	SDL_Log("bit_test = %s\n", bit_test.to_string().c_str());
	SDL_Log("bit_and  = %s\n", bit_and.to_string().c_str());
	SDL_Log("bit_res  = %s\n", bit_res.to_string().c_str());

	SDL_Log("Person is POD = %d\n", std::is_pod<Person>::value);
	SDL_Log("Something is POD = %d\n", std::is_pod<Something>::value);
	SDL_Log("ComponentId is POD = %d\n", std::is_pod<ComponentId>::value);

	Something s = { 1, 2, "hi" };
	auto s0 = getMemberVal<Something, SomethingPred0>(s, SomethingPred0{});
	SDL_Log("s val 0 through template = %d\n", s0);

	// output person properties
	SDL_Log("%s {\n", Person::Reflection::getClassType().c_str());
	for (auto p : Person::Reflection::getProperties()) {
		SDL_Log("  %s %s : %s\n", FieldTypeToString(p.type), p.name.c_str(), p.description.c_str());
	}
	SDL_Log("}\n\n");

	// testing tuple for_each
	//using namespace std;
	using namespace boost::fusion;
	for_each(vals, print{});
	std::ostringstream oss;
	oss << vals;
	SDL_Log("\n\n%s\n\n", oss.str().c_str());

	std::ofstream ofs;
	ofs.open("test.txt", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	ofs.write((const char*)&s, sizeof(s));
	ofs.close();
}
