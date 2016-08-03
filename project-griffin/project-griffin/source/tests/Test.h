#pragma once
#ifndef GRIFFIN_TEST_H_
#define GRIFFIN_TEST_H_

#include <vector>
#include <memory>
#include <utility/Logger.h>

namespace griffin {
	namespace test {

		//#define REGISTER_TEST(TestClass) std::unique_ptr<test::Test> instance = std::make_unique<TestClass>();
		//#define REGISTER_TEST(TestClass) test::Test* instance = new TestClass();


		class Test {
		public:
			explicit Test();
			virtual ~Test() {}
			virtual void run(Logger&) = 0;
		};


		class TestRunner {
		public:
			TestRunner(Logger& _log);

			void runAllTests();

			static std::shared_ptr<Test> registerTest(const std::shared_ptr<Test>& testPtr) {
				s_testRegistry.push_back(testPtr);
				return testPtr;
			}

		private:
			static std::vector<std::shared_ptr<Test>> s_testRegistry;

			Logger& log;
		};


		class ConcurrencyTest : public Test {
		public:
			explicit ConcurrencyTest() : Test() {}

			void run(Logger& log);
		};
	}
}
#endif