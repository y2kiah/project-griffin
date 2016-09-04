#pragma once
#ifndef GRIFFIN_TEST_H_
#define GRIFFIN_TEST_H_

#include <vector>
#include <memory>
#include <functional>

namespace griffin {
	namespace test {

		/*class Test {
		public:
			virtual ~Test() {}
			virtual void run() = 0;
		};*/


		class TestRunner {
		public:
			void runAllTests();
			void registerAllTests();

		private:
			static std::vector<std::function<void()>> s_testRegistry;
		};
	}
}
#endif