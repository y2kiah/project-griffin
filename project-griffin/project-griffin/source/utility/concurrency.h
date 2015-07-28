/**
* @file concurrency.h
* @author Jeff Kiah
* Two neat classes from Herb Sutter that wrap any other class of type T and synchronizes every
* member function call, effectively making the wrapped class thread-safe. The wrapping pattern
* used is interesting by itself. The concurrent class uses a concurrent_queue and worker thread to
* get asynchronous non-blocking behavior, returning a std::future. The monitor class uses a simple
* for thread safety, but should hardly ever be used because it both over-locks (every method is
* too much) and under-locks (no transaction-level locking) in most real situations. It can maybe
* be used for printf or cout logging, but even then a concurrent_queue is preferrable. The
* wrapping pattern can also be used in lieu of inheritance in many situations.
* @see http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
*/
#pragma once
#ifndef GRIFFIN_CONCURRENCY_H_
#define GRIFFIN_CONCURRENCY_H_

#include <array>
#include <thread>
#include <mutex>
#include <future>
#include <functional>
#include <bitset>
#include <utility/container/concurrent_queue.h>

#include <SDL_log.h>

namespace griffin {

	using std::atomic;

#define CONCURRENT_MAX_WORKER_THREADS	8
#define CONCURRENT_NUM_FIXED_THREADS	3
// one shared by all worker threads, plus one each for fixed thread affinity
#define CONCURRENT_NUM_TASK_QUEUES		CONCURRENT_NUM_FIXED_THREADS + 1
	
	enum ThreadAffinity : uint8_t {
		Thread_Workers = 0,
		Thread_OS_Input,
		Thread_Update,
		Thread_OpenGL_Render
	};

	/**
	* TODO: seems to be a join bug when CONCURRENT_MAX_WORKER_THREADS is 8 on work machine
	*/
	class thread_pool {
	public:
		typedef std::array<std::thread, CONCURRENT_MAX_WORKER_THREADS> ThreadList;
		typedef std::array<concurrent_queue<std::function<void()>>, CONCURRENT_NUM_TASK_QUEUES> TaskQueueList;

		explicit thread_pool(int cpuCount)
		{
			//assert(m_busy.is_lock_free());

			// start up one worker thread per core
			m_numWorkerThreads = cpuCount > CONCURRENT_MAX_WORKER_THREADS ? CONCURRENT_MAX_WORKER_THREADS : cpuCount;
			
			auto threadProcess = [=]{
				while (!m_done) {
					m_tasks[Thread_Workers].wait_pop()();
				}
				SDL_Log("exiting thread %llu", std::this_thread::get_id().hash());
			};

			for (int i = 0; i < m_numWorkerThreads; ++i) {
				m_threads[i] = std::thread{ threadProcess };
			}
		}
		
		thread_pool(const thread_pool&) = delete; // can't copy a thread_pool

		~thread_pool() {
			SDL_Log("thread pool deleted");
			m_tasks[Thread_Workers].clear();
			m_done = true;
			for (int i = 0; i < m_numWorkerThreads; ++i) {
				SDL_Log("pushing done task");
				m_tasks[Thread_Workers].push([=]{
					m_done = true;
				});
			}

			for (auto& t : m_threads) {
				if (t.joinable()) {
					SDL_Log("joining thread %llu", t.get_id().hash());
					t.join();
				}
			}
		}
		
		/**
		* @returns true if thread_pool will run the task, false if thread_pool rejects the task
		*		because its destructor has already been called (engine is exiting)
		*/
		bool run(ThreadAffinity affinity, std::function<void()>&& f) {
			if (!m_done) {
				m_tasks[affinity].push(std::forward<std::function<void()>>(f));
				return true;
			}
			return false;
		}

		// Implement FIFO scheduling, with a thread affinity system
		//   If affinity is set the thread will prefer to execute that task and leave the
		//   general task it skips to be executed by a worker thread. In that way, tasks which
		//   specify affinity for a fixed thread will take priority. Affinity should only be used
		//   when absolutely necessary (like for instance calls to OpenGL or other single-threaded
		//   libraries are made).
		// Each worker thread waits on a condition variable, when a task shows up notify_one is
		//   called. Worker threads pull from the general task queue only.
		// The Fixed threads are not owned by the thread-pool itself, but participate by taking the
		//   tasks that specify affinity for them, and (optionally) share in executing general
		//   tasks at some pre-determined point of their own loop.
		// Only fixed threads can be specified for affinity. Fixed threads should use try_pop to
		//   pull tasks from the queue, whereas worker threads should wait on the queue's condition
		//   variable using wait_pop.

	private:
		ThreadList		m_threads;
		TaskQueueList	m_tasks;
		//std::atomic<std::bitset<MAX_WORKER_THREADS>> m_busy = 0;
		int8_t			m_numWorkerThreads = 0;
		atomic<bool>	m_done = false;
		
	};

	typedef std::shared_ptr<thread_pool> ThreadPoolPtr;


	class task_base {
	public:
		static ThreadPoolPtr s_threadPool;	//<! declared in Engine.cpp
	};

	/**
	*
	*/
	template <typename ResultType>
	class task : public task_base {
	public:
		// Type Definitions

		typedef ResultType result_type;
		typedef std::promise<result_type> promise_type;
		typedef std::shared_future<result_type> future_type;
		enum Flags : uint8_t {
			Task_None       = 0,
			Task_Valid      = 1,	//<! task is valid if it is runnable and not default constructed, must have called run or created as a continuation
			Task_Run_Called = 2		//<! run function called, or continuation run
		};

		// Variables

		struct Impl {
			promise_type	p;				//<! promise for result of task
			future_type		result;			//<! future return value of the task
			ThreadAffinity	threadAffinity;	//<! thread affinity for scheduling the task
			uint8_t			flags;			//<! contains flags for this task
			std::function<void(ThreadAffinity)> fCont;	//<! continuation capture
		};
		std::shared_ptr<Impl> _pImpl;

		// Functions

		task(ThreadAffinity threadAffinity_ = Thread_Workers) :
			_pImpl(std::make_shared<Impl>())
		{
			_pImpl->result = _pImpl->p.get_future();
			_pImpl->threadAffinity = threadAffinity_;
			_pImpl->flags = Flags::Task_None;
		}

		task(task&& t) :
			_pImpl(std::move(t._pImpl))
		{
			t._pImpl.reset();
		}

		task(const task& t) :
			_pImpl(t._pImpl)
		{}

		task<result_type>& operator=(task<result_type>&& t)
		{
			if (&t != this) {
				_pImpl = t._pImpl;
				t._pImpl.reset();
			}
			return *this;
		}

		task<result_type>& operator=(const task<result_type>& t)
		{
			_pImpl = t._pImpl;
			return *this;
		}


		/**
		* Executes function pointer, functor or lambda on the worker thread where the task is
		* internally serialized by the concurrent queue for thread safety.
		* @tparam	F		function pointer, functor or lambda accepting one argument of type T
		* @param	func	type F
		* @returns future_type
		*/
		template <typename F, typename...Args>
		task<result_type>& run(F func, Args...args) {
			assert(!run_called() && "run called twice, invalid usage");
			_pImpl->flags |= Flags::Task_Valid | Flags::Task_Run_Called;

			auto pImpl = _pImpl;
			bool run = s_threadPool->run(_pImpl->threadAffinity, [pImpl, func, args...]{
				auto& impl = *pImpl;
				try {
					set_value(impl.p, func, args...);
				}
				catch (...) {
					impl.p.set_exception(std::current_exception());
				}
				// call the continuation, it will run immediately if theadAffinity is the same,
				// otherwise it will queue up in the thread pool
				if (impl.fCont) { impl.fCont(impl.threadAffinity); }
			});

			// if thread pool is exiting, 
			if (!run) {
				_pImpl->p.set_exception(std::make_exception_ptr(std::logic_error("task submitted after exit")));
			}

			return *this;
		}
		
		void wait() const {
			_pImpl->result.wait();
		}

		result_type get() const {
			return _pImpl->result.get();
		}

		future_type get_future() {
			return _pImpl->result;
		}

		bool is_ready() const {
			return _pImpl->result._Is_ready();
		}

		bool is_valid() const {
			return (_pImpl->flags & Flags::Task_Valid) != 0;
		}

		bool run_called() const {
			return (_pImpl->flags & Flags::Task_Run_Called) != 0;
		}

		bool has_continuation() const {
			return !!_pImpl->fCont; // coerce compiler to run the bool operator
		}
		
		template <typename Fc>
		auto then(Fc fCont_, ThreadAffinity threadAffinity_ = Thread_Workers) -> task<decltype(fCont_())> {
			task<decltype(fCont_())> continuation(threadAffinity_);
			continuation._pImpl->flags |= Flags::Task_Valid;

			auto& thisImpl = *_pImpl;
			if (thisImpl.result._Is_ready()) {
				// task has already finished, run the continuation
				continuation.run(std::move(fCont_));
			}
			else {
				thisImpl.fCont = [continuation, fCont_](ThreadAffinity previousThreadAffinity) {
					auto& impl = *continuation._pImpl;

					if (impl.threadAffinity == previousThreadAffinity) {
						impl.flags |= Flags::Task_Run_Called;

						// run continuation immediately if it can run on the same thread
						try {
							set_value(impl.p, fCont_);
						}
						catch (...) {
							impl.p.set_exception(std::current_exception());
						}
						if (impl.fCont) { impl.fCont(impl.threadAffinity); }
					}
					else {
						// push to the thread pool
						const_cast<task<decltype(fCont_())>&>(continuation).run(std::move(fCont_));
					}
				};
			}
			return continuation;
		}

	private:
		/**
		* Helpers for p.set_value() so the function above compiles for both void and non-void.
		*/
		template <typename TFut, typename F, typename...Args>
		static void set_value(std::promise<TFut>& p, F& f, Args...args) {
			p.set_value(f(args...));
		}

		template <typename F, typename...Args>
		static void set_value(std::promise<void>& p, F& f, Args...args) {
			f(args...);
			p.set_value();
		}
	};

	/*template <typename Iterator,
			  typename std::enable_if<!std::is_void<typename std::iterator_traits<Iterator>::value_type::result_type>::value>::type* = 0>
	auto when_all(Iterator first, Iterator last) -> task<std::vector<typename std::iterator_traits<Iterator>::value_type::result_type>>
	{
		task<std::vector<typename std::iterator_traits<Iterator>::value_type::result_type>> tsk;

		tsk.run([first, last]{
			std::vector<typename std::iterator_traits<Iterator>::value_type::result_type> ret;
			//ret.reserve();
			
			for (auto i = first; i != last; ++i) {
				ret.push_back(i->get());
			}

			return ret;
		});

		return tsk;
	}*/

	/*template <typename...Tasks>
	task<void> when_all(Tasks...tasks) {
		task<void> ret;

		ret.run([tasks...]{
			
			//for (int i = 0; i < sizeof...(Tasks); ++i) {
			//}
		});

		return ret;
	}*/

	/*
	template <typename F, typename... Tasks>
	task<void> when_any(F&& f, Tasks&...) {
		task<void> ret;
		ret.run([]{
			return f();
		});

		return ret;
	}
	*/

	/*
	template <typename T>
	class task_group {
	public:
		// chaining methods
		//  then - maybe this is a free function taking a packaged_task and returning a task_group?
		//  when_all
		//  when_any
		
		// start/stop methods
		//  run
		//  cancel
		//  get
		//  wait

	private:
		vector<packaged_task<T>> m_tasks;
	};
	*/

	/**
	* @class concurrent
	* The concurrent class wraps any class T and accepts lambdas that take a reference to the
	* contained object for performing operations (via member-function calls presumably) in a thread
	* safe manner. Lambdas are queued by chaining task continuations without occupying a thread
	* while waiting. This is the non-blocking asychronous way to wrap a class for thread safety.
	* This should wrap a high level class with few instances. See the backgrounder class example
	* below for an illustration.
	* @tparam	T	type of class or data wrapped for concurrency
	*/
	template <typename T>
	class concurrent2 {
	public:
		concurrent2(T t_ = T{}) :
			t(std::move(t_))
		{}

		/**
		* Executes function pointer, functor or lambda on a worker thread (or thread specified by
		* affinity where the task is internally serialized for thread safety by chaining task
		* continuations, without blocking and without occupying a worker thread while waiting.
		* @tparam	F	function pointer, functor or lambda accepting one argument of type T
		* @param	f	type F
		* @param	threadAffinity_	set thread to run the function
		* @returns std::future of type T
		*/
		template <typename F>
		auto operator()(F f, ThreadAffinity threadAffinity_ = Thread_Workers) const -> std::shared_future<decltype(f(t))>
		{
			std::unique_lock<mutex> lock(m_mutex);

			if (!m_latestTask.is_valid()) {
				// this runs on the first call
				task<decltype(f(t))> newTask(threadAffinity_);

				//newTask.run(f, t); // this not working, why? example shared_ptr ends up null in resource cache
				newTask.run([f, this]{
					return f(t);
				});

				m_latestTask = newTask.then([]{}, threadAffinity_);

				return newTask.get_future();
			}
			else {
				// this runs on subsequent calls
				auto newTask = m_latestTask.then([f, this]{
					return f(t); // it's legal to return void http://stackoverflow.com/questions/2249108/can-i-return-in-void-function
				}, threadAffinity_);
				
				m_latestTask = newTask.then([]{}, threadAffinity_);

				return newTask.get_future();
			}
		}

	private:
		mutable T			t;
		mutable mutex		m_mutex;
		mutable task<void>	m_latestTask;
	};


	template <typename T>
	class concurrent {
	public:
		concurrent(T t_ = T{}) :
			t(std::move(t_)),
			worker{ [=]{ while (!done) { q.wait_pop()(); } } }
		{}

		~concurrent() {
			q.push([=]{ done = true; });
			worker.join();
		}

		template <typename F>
		auto operator()(F f) const -> std::shared_future<decltype(f(t))> {
			auto p = std::make_shared<std::promise<decltype(f(t))>>();

			q.push([=]{
				try {
					set_value(*p, f, t);
				}
				catch (...) {
					p->set_exception(std::current_exception());
				}
			});

			return p->get_future();
		}

	private:
		mutable T t;
		mutable concurrent_queue<std::function<void()>> q;
		bool done = false;
		std::thread worker;

		/**
		* Helpers for p.set_value() so the function above compiles for both void and non-void.
		*/
		template <typename Fut, typename F, typename T>
		static void set_value(std::promise<Fut>& p, F& f, T& t) {
			p.set_value(f(t));
		}

		template <typename F, typename T>
		static void set_value(std::promise<void>& p, F& f, T& t) {
			f(t);
			p.set_value();
		}
	};


	/**
	* @class monitor
	* The monitor class wraps any class T and accepts lambdas that take a reference to the
	* contained object for performing operations (via member-function calls presumably) in a thread
	* safe manner. Invoked lambdas are internally synchronized using a mutex, so this would
	* potentially block client threads and cause contention. This should almost never be used,
	* prefer the concurrent<T> class and task parallelism when possible.
	* @tparam	T	class or data wrapped to be protected by a mutex for thread safety
	*/
	template <typename T>
	class monitor {
	public:
		monitor(T t_ = T{}) : t(t_) {}

		/*template <typename F>
		auto operator()(F f) const -> decltype(f(t)) {
		std::lock_guard<std::mutex> _{ m };
		return f(t);
		}*/

		template <typename F>
		auto operator()(F f) const -> typename std::result_of<F(T)>::type {
			std::lock_guard<std::mutex> _{ m };
			return f(t);
		}

	private:
		mutable T t;
		mutable std::mutex m;
	};


	/**
	* Example usage of concurrent<T>.
	* @code
	*	concurrent<string> cs;
	*	auto f = cs([](string& s) {		// C++14 polymorphic lambda becomes cs([](s) {
	*		// thread safe transaction!
	*		s += "added thread-safe";	// this is operating on the internal (wrapped) object
	*		return string("this was ") + s;
	*	});
	*	// f is a std::future for the lambda return value
	* @endcode
	*/

	/**
	* Example usage of concurrent<T>.
	* One way to use the concurrent<t> class is to protect a private data member of an outer class
	* (call it backgrounder for example) that exposes a simple public API. That way the weird
	* usage pattern of submitting lambdas can be hidden within the implementation of the public
	* APIs, and client code will not have to know or care about the concurrent<t> usage pattern.
	* @code
	*	class backgrounder {
	*	public:
	*		std::future<bool> save(std::string file) {
	*			c([=](data& d) {
	*				// each function is an ordered transaction
	*			});
	*		}
	*
	*		std::future<size_t> print(some& stuff) {
	*			c([=, &stuff](data& d) {
	*				// atomic and indivisible w.r.t. other functions
	*			});
	*		}
	*
	*	private:
	*		struct data {  };	// private data is unshared by construction
	*		concurrent<data> c;	// fully thread-safe internal data
	*	};
	* @endcode
	*/

	/**
	* Example usage of monitor<T>.
	* @code
	*	monitor<string> s;
	*	auto f = s([](string& s) {		// C++14 polymorphic lambda becomes s([](s) {
	*		// thread safe transaction!
	*		s += "added thread-safe";	// this is operating on the internal (wrapped) object
	*		return string("this was ") + s;
	*	});
	*	// f is a string (note that with concurrent<t> this would be a future)
	* @endcode
	*/
}
#endif