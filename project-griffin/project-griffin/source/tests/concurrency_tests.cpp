#include "Test.h"
#include <utility/Logger.h>
#include <utility/concurrency.h>

#include <SDL_log.h>


using namespace griffin;

/*class ConcurrencyTest : public test::Test {
public:
	explicit ConcurrencyTest() : Test() {}
	*/
	void test::ConcurrencyTest::run(Logger& log) {
		// test concurrency system
		griffin::task<int> tsk;
		tsk.run([]{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			SDL_Log("task 1 step 1");
			return 1;
		});
		auto& fut2 = tsk.then([]{
			SDL_Log("task 1 step 2");
			return 2.0f;
		});

		griffin::task<void> tsk2;
		tsk2.run([](std::shared_future<int> tsk1Fut, float f){
			SDL_Log("task 2 step 1, see value %d, %.1f", tsk1Fut.get(), f);
		}, tsk.get_future(), 3.0f) // to support non-lambda functions, this syntax can be used to pass a parameter instead of capturing it
		.then([]{
			std::this_thread::sleep_for(std::chrono::seconds(5));
			SDL_Log("task 2 step 2");
		})
		.then([]{
			SDL_Log("task 2 step 3 OpenGL");
		}, ThreadAffinity::Thread_OpenGL_Render);

		fut2.then([fut2]{ // when using lambda you can capture a previous step's task/future and get its value
			SDL_Log("task 1 step 3, see value %.1f", fut2.get());
		})
		.then([]{
			SDL_Log("task 1 step 4");
		});

		griffin::task<int> tsk3;
		tsk3.run([]{
			SDL_Log("task 3");
			return 3;
		});

			// we can group up tasks with the same return type into a container for when_all/when_any
		task<int> whenAllTasks[2] = { tsk, tsk3 };
	
		when_all<task<int>,2>(whenAllTasks/*std::begin(whenAllTasks), std::end(whenAllTasks)*/)
		.then([tsk, tsk3]{
			SDL_Log("when_all finished");
			SDL_Log("  tsk  value = %d", tsk.get());
			SDL_Log("  tsk3 value = %d", tsk3.get());
		});
	
		/*when_all(tsk, tsk2, tsk3).then([]{
			SDL_Log("when_all finished");
		});*/
	}
//};

//REGISTER_TEST(ConcurrencyTest)