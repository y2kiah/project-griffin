#include "Component.h"
#include <tuple>
#include <SDL_log.h>

void test_reflection() {
	Person person = { 32, 10.0f, { 1, 2, 3 } };

	auto & personProps = Person::Reflection::getProperties();
	auto vals = Person::Reflection::getAllValues(person);

	SDL_Log("%s:\n", Person::Reflection::getClassType().c_str());
	for (const auto &f : personProps) {
		SDL_Log("%s %s : %s\n", FieldTypeToString(f.type), f.name.c_str(), f.description.c_str());
	}
	int a = std::get<Person::Reflection::age>(vals);
	float b = std::get<Person::Reflection::speed>(vals);
	auto c = std::get<Person::Reflection::stuff>(vals);

	ComponentMask bit_test;
	bit_test.set(ComponentType::Position_T, true);
	ComponentMask bit_and(0xFFFFFFFF);
	ComponentMask bit_res = bit_test | bit_and;
	SDL_Log("bit_test = %s\n", bit_test.to_string().c_str());
	SDL_Log("bit_and  = %s\n", bit_and.to_string().c_str());
	SDL_Log("bit_res  = %s\n", bit_res.to_string().c_str());

	struct Something { int x; };
	SDL_Log("Person is POD = %d\n", std::is_pod<Person>::value);
	SDL_Log("Something is POD = %d\n", std::is_pod<Something>::value);
}