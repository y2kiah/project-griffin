#include "Test.h"
#include <utility/Logger.h>

using namespace griffin;
using namespace griffin::test;

#define REGISTER_TEST(testFunc)	extern void testFunc();\
								s_testRegistry.push_back(testFunc)

std::vector<std::function<void()>> TestRunner::s_testRegistry;


void TestRunner::runAllTests()
{
	for (auto& t : s_testRegistry) {
		t();
	}
}

void TestRunner::registerAllTests()
{
	#pragma warning(disable : 4101)
	
	// register all tests in this section
	//REGISTER_TEST(concurrencyTest);
	//REGISTER_TEST(testHandleMap);
	//REGISTER_TEST(testReflection);
}