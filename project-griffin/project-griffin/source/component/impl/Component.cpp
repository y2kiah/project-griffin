#include <component/components.h>
#include <vector>
#include <tuple>
#include <SDL_log.h>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <utility/prettyprint.h>
#include <fstream>
#include <memory>
#include <utility/Profile.h>

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

// example cast objects
template <int Ct>
struct component_type {
	typedef void type;
};

template <>
struct component_type<2> {
	typedef Person* type;
};

template <typename T, int Ct>
auto component_cast(T* x) -> decltype(component_type<Ct>::type)
{
	return reinterpret_cast<component_type<Ct>::type>(x);
}

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
	componentIds = personStore.createComponents(numTestComponents);
	timer.stop();
	SDL_Log("**********\ncreate components in store\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());

	timer.start();
	// store 100,000 components on the heap
	for (int i = 0; i < numTestComponents; ++i) {
		personHeap.push_back(std::unique_ptr<Person>(new Person{0}));
	}
	timer.stop();
	SDL_Log("**********\ncreate components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
}

void profileTestComponents() {
	int age = 0;

	personStore.getComponent(componentIds.back()).age = 10;
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personStore.getComponent(componentIds[i]).age;
	}
	timer.stop();
	SDL_Log("**********\nloop components with external id\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age); // use age so it doesn't compile away in release build test

	auto cmp = personStore.getComponents().getItems();
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += cmp[i].age;
	}
	timer.stop();
	SDL_Log("**********\nloop components inner array\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());
	SDL_Log("age=%d\n", age);

	personHeap.back()->age = 5;
	timer.start();
	for (int i = 0; i < numTestComponents; ++i) {
		age += personHeap[i]->age;
	}
	timer.stop();
	SDL_Log("**********\nloop components on heap\ntime = %f ms\ncounts = %lld\n\n", timer.millisecondsPassed(), timer.countsPassed());

	SDL_Log("age=%d\n", age);
}

void test_reflection() {
	addTestComponents();
	profileTestComponents();

	SDL_Log(personStore.to_string().c_str());
	Person& person = personStore.getComponent(componentIds[0]);
	
	auto& personProps = Person::Reflection::getProperties();
	personProps.emplace_back(FieldType::bool_T, "added", "added this to properties at runtime", sizeof(bool));
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
		SDL_Log("  %s %s (%d): %s\n", FieldTypeToString(p.type), p.name.c_str(),
				p.size, p.description.c_str());
	}
	SDL_Log("}\n\n");

	// testing tuple for_each
	using namespace boost::fusion;
	std::ostringstream oss;
	oss << vals;
	SDL_Log("\n\n%s\n\n", oss.str().c_str());

	std::ofstream ofs;
	ofs.open("test.txt", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
	ofs.write(reinterpret_cast<const char*>(&s), sizeof(s));
	ofs.close();

	// testing ComponentId comparisons
	// we want the ComponentType portion to sort at a higher priority than the index or generation,
	vector<ComponentId> sortIds = {
			{{{0, 0, ComponentType::Person_T, 0}}},
			{{{0, 0, ComponentType::Orientation_T, 0}}},
			{{{0xFFFFFFFF, 0xFFFF, ComponentType::Orientation_T, 0}}},
			{{{50, 1, ComponentType::Person_T, 1}}},
			{{{5, 1, ComponentType::Person_T, 0}}},
			{{{5, 1, ComponentType::Position_T, 0}}},
			{{{5, 3, ComponentType::Orientation_T, 0}}}
		};
	std::sort(sortIds.begin(), sortIds.end());
	oss.clear();
	oss << sortIds;
	SDL_Log(oss.str().c_str());
}
