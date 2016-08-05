#include "Test.h"
#include <utility/Logger.h>
#include <utility/concurrency.h>

using namespace griffin;

void concurrencyTest()
{
	// test concurrency system
	griffin::task<int> tsk;
	tsk.run([]{
		std::this_thread::sleep_for(std::chrono::seconds(10));
		logger.test("task 1 step 1");
		return 1;
	});
	auto& fut2 = tsk.then([]{
		logger.test("task 1 step 2");
		return 2.0f;
	});

	griffin::task<void> tsk2;
	tsk2.run([](std::shared_future<int> tsk1Fut, float f){
		logger.test("task 2 step 1, see value %d, %.1f", tsk1Fut.get(), f);
	}, tsk.get_future(), 3.0f) // to support non-lambda functions, this syntax can be used to pass a parameter instead of capturing it
	.then([]{
		std::this_thread::sleep_for(std::chrono::seconds(5));
		logger.test("task 2 step 2");
	})
	.then([]{
		logger.test("task 2 step 3 OpenGL");
	}, ThreadAffinity::Thread_OpenGL_Render);

	fut2.then([fut2]{ // when using lambda you can capture a previous step's task/future and get its value
		logger.test("task 1 step 3, see value %.1f", fut2.get());
	})
	.then([]{
		logger.test("task 1 step 4");
	});

	griffin::task<int> tsk3;
	tsk3.run([]{
		logger.test("task 3");
		return 3;
	});

	std::array<task<int>,2> whenAllTasks = { tsk, tsk3 };
	// we can group up tasks with the same return type into a container for when_all/when_any
	when_all(whenAllTasks)//std::begin(whenAllTasks), std::end(whenAllTasks))
	.then([tsk, tsk3]{
		logger.test("when_all finished");
		logger.test("  tsk  value = %d", tsk.get());
		logger.test("  tsk3 value = %d", tsk3.get());
	});

	/*when_all(tsk, tsk2, tsk3).then([]{
		logger.test("when_all finished");
	});*/
}
