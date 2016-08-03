#pragma once
#ifndef GRIFFIN_TEST_H_
#define GRIFFIN_TEST_H_

#include <vector>
#include <memory>
#include <utility/Logger.h>

namespace griffin {
	namespace test {

		class Test {
		public:
			virtual ~Test() {}
			virtual void run(Logger&) = 0;
		};


		class TestRunner {
		public:
			void runAllTests();
			void registerAllTests(Logger& log);

		private:
			static std::vector<std::function<void()>> s_testRegistry;
		};
	}
}
#endif