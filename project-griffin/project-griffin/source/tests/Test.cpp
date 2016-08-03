#include "Test.h"

using namespace griffin;
using namespace griffin::test;

#define REGISTER_TEST(testFunc)	extern void testFunc(Logger&);\
								s_testRegistry.push_back([&log](){\
									concurrencyTest(log);\
								});

std::vector<std::function<void()>> TestRunner::s_testRegistry;


void TestRunner::runAllTests()
{
	for (auto& t : s_testRegistry) {
		t();
	}
}

void TestRunner::registerAllTests(Logger& log)
{
	#pragma warning(disable : 4101)
	
	// register all tests in this section
	REGISTER_TEST(concurrencyTest)

}