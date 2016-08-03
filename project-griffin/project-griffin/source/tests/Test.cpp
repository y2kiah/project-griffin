#include "Test.h"

using namespace griffin;
using namespace griffin::test;


std::vector<std::shared_ptr<Test>> TestRunner::s_testRegistry;


Test::Test() {
//	g_testRegistry[0] = this;// .push_back(this);
}


TestRunner::TestRunner(Logger& _log) :
	log{ _log }
{}

void TestRunner::runAllTests() {
	for (auto& t : s_testRegistry) {
		t->run(log);
	}
}