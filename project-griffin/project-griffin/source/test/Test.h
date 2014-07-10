#pragma once
#ifndef TEST_H
#define TEST_H

#include <vector>
#include <string>
#include <functional>

/*
* Example:
	TestRunner::add("TestName", "TestGroup", [=](Test& test, std::ostream& output) mutable {
		// test specific code here
		output << "write test output";
	});
*/

using std::vector;
using std::string;

namespace Test {
	enum Status {
		Pending,	// test is waiting to run
		Running,	// test is running
		Passed,		// test finished, passed all expectations
		Failed,		// test finished, failed one or more expectations
		Undefined	// test is not initialized
	};
	
	/**
	*
	*/
	struct expect {
		static bool ok(bool result) {
			return result;
		}

		template <typename T, typename U>
		static bool equal(T t, U u) {
			return (t == u);
		}
	};

	struct Expectation {

	};

	/**
	*
	*/
	struct Test {
		Test(const Test&) = default;
		Test(Test&& t) : status(Undefined), func(nullptr) {
			status = t.status;
			func = std::move(t.func);
			t.func = nullptr;
		}

		Status status = Undefined;
		std::function<int(Test&, std::ostream&)> func;
		vector<Expectation> expectations;
	};

	/**
	*
	*/
	class TestRunner {
	public:
		void runAllTests() {
			for (auto t : tests) {
				t.func(t, output);
			}
		}

	private:
		vector<Test> tests;
		std::ostream output;
	};

}
#endif