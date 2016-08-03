/**
* @file concurrency.h
* @author Jeff Kiah
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
#include <utility/memory_reserve.h>

#include <SDL_log.h>

namespace griffin {

	using std::atomic;

#define CONCURRENT_MAX_WORKER_THREADS	16
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
		typedef std::array<std::vector<std::function<void()>>, CONCURRENT_NUM_FIXED_THREADS> TaskPopList;

		explicit thread_pool(int cpuCount)
		{
			//assert(m_busy.is_lock_free());

			// reserve memory for fixed thread pop lists
			for (auto& pt : m_popTasks) {
				pt.reserve(RESERVE_CONCURRENCY_POP_TASK_LIST);
			}

			// start up one worker thread per core
			m_numWorkerThreads = cpuCount > CONCURRENT_MAX_WORKER_THREADS ? CONCURRENT_MAX_WORKER_THREADS : cpuCount;
			
			auto threadProcess = [=]{
				while (!m_done) {
					m_tasks[Thread_Workers].wait_pop()();
				}
			};

			for (int i = 0; i < m_numWorkerThreads; ++i) {
				m_threads[i] = std::thread{ threadProcess };
			}
		}
		
		thread_pool(const thread_pool&) = delete; // can't copy a thread_pool

		/**
		* Thread pool destructor clears the workers queue and joins all worker threads
		*/
		~thread_pool() {
			SDL_Log("thread pool deleted");

			m_tasks[Thread_Workers].clear();
			m_done = true;
			
			// push a "done" task for each worker thread
			for (int i = 0; i < m_numWorkerThreads; ++i) {
				SDL_Log("pushing done task");
				m_tasks[Thread_Workers].push([=]{
					m_done = true;
				});
			}

			// join all worker threads
			for (auto& t : m_threads) {
				if (t.joinable()) {
					SDL_Log("joining thread %llu", t.get_id().hash());
					t.join();
				}
			}

			// checked pop task lists for reserve capacity overflow
			for (auto& pt : m_popTasks) {
				if (pt.capacity() > RESERVE_CONCURRENCY_POP_TASK_LIST) {
					SDL_Log("check RESERVE_CONCURRENCY_POP_TASK_LIST: original=%d, highest=%d", RESERVE_CONCURRENCY_POP_TASK_LIST, pt.capacity());
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

		/**
		* @threadAffinity_	which thread's tasks to run, corresponds with calling thread, value
		*		cannot be ThreadAffinity::Thread_Workers
		* @maxNumber	maximum number of tasks to run, 0 = unlimited
		*/
		void executeFixedThreadTasks(ThreadAffinity threadAffinity_, int maxNumber = 0)
		{
			assert(threadAffinity_ != ThreadAffinity::Thread_Workers && "can't run work thread tasks with this function");

			auto& popTasks = m_popTasks[threadAffinity_ - 1];

			m_tasks[threadAffinity_].try_pop_all(popTasks, maxNumber);
			
			for (auto& task : popTasks) {
				task();
			}

			popTasks.clear();
		}


		/**
		* Data-parallel jobs may want to know how many worker threads there are, to divide up the
		* work evenly.
		*/
		int getNumWorkerThreads() const
		{
			return m_numWorkerThreads;
		}

		// Implements FIFO scheduling, with a thread affinity system
		//   If affinity is set a thread will execute that task first and leave the worker tasks
		//   it skips to be executed by a worker thread. In that way, tasks that specify affinity
		//   for a fixed thread will take priority. Affinity should only be used when absolutely
		//   necessary (like for calls to OpenGL or other thread-specific libraries).
		// Each worker thread waits on a condition variable, when a task shows up notify_one is
		//   called. Worker threads pull from the general task queue only.
		// The Fixed threads are not owned by the thread-pool itself, but participate by taking the
		//   tasks that specify affinity for them, and (optionally) share in executing general
		//   tasks at some pre-determined point of their own loop.
		// Only fixed threads can be specified for affinity. Fixed threads use try_pop to pull
		//   tasks from the queue, whereas worker threads wait on the queue's condition variable
		//   using wait_pop.

	private:
		ThreadList		m_threads;		//<! worker threads owned by the thread_pool
		TaskQueueList	m_tasks;		//<! concurrent_queues for pushing
		TaskPopList		m_popTasks;		//<! vectors for popping tasks from the queue to be executed on fixed threads
		//std::atomic<std::bitset<MAX_WORKER_THREADS>> m_busy = 0;
		atomic<bool>	m_done = false;
		int8_t			m_numWorkerThreads = 0;
		
	};

	typedef std::shared_ptr<thread_pool> ThreadPoolPtr;


	/**
	*
	*/
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
			std::function<void(ThreadAffinity)> fCont;	//<! continuation capture
			uint8_t			flags;			//<! contains flags for this task
		};
		std::shared_ptr<Impl> _pImpl;

		// Functions

		task(ThreadAffinity threadAffinity_ = Thread_Workers) :
			_pImpl(std::make_shared<Impl>())
		{
			auto& impl = *_pImpl;
			impl.result = impl.p.get_future();
			impl.threadAffinity = threadAffinity_;
			impl.flags = Flags::Task_None;
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


	// working, but the return vector is not necessary
	/*template <typename Iterator,
			  typename std::enable_if<!std::is_void<typename std::iterator_traits<Iterator>::value_type::result_type>::value>::type* = 0>
	auto when_all(Iterator first, Iterator last) -> task<std::vector<typename std::iterator_traits<Iterator>::value_type::result_type>>
	{
		typedef std::vector<typename std::iterator_traits<Iterator>::value_type::result_type>	result_type_set;
		typedef std::vector<typename std::iterator_traits<Iterator>::value_type>				value_type_set;

		task<result_type_set> newTask;
		
		value_type_set tasks(first, last);

		// newTask.run([tasks](){		// C++14-compatible move capture
		newTask.run([tasks](){
			result_type_set ret;
			ret.reserve(tasks.size());

			for (auto& t : tasks) {
				ret.push_back(t.get());
			}

			return ret;
		});

		return newTask;
	}*/

	template <typename Iterator>
	task<void> when_all(Iterator first, Iterator last)
	{
		typedef std::vector<typename std::iterator_traits<Iterator>::value_type> value_type_set;

		task<void> newTask;
		
		value_type_set tasks(first, last);

		// newTask.run([&&tasks](){		// C++14-compatible move capture
		newTask.run([tasks](){
			for (auto& t : tasks) {
				t.wait();
			}
		});

		return newTask;
	}


	template <typename Task, size_t N>
	task<void> when_all(std::array<Task, N>& tasks)
	{
		task<void> newTask;

		// newTask.run([&&tasks](){		// C++14-compatible move capture
		newTask.run([tasks](){
			for (int t = 0; t < N; ++t) {
				tasks[t].wait();
			}
		});

		return newTask;
	}


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


	/**
	* @class serialize
	* The serialize class wraps any class T and accepts lambdas that take a reference to the
	* contained object for performing operations (via member-function calls) in a thread-safe
	* manner. Lambdas are queued by chaining task continuations without occupying a thread
	* while waiting. This is the non-blocking asychronous way to wrap a class for thread safety.
	* @tparam	T	type of class or data wrapped for concurrency
	*/
	template <typename T>
	class serialize {
	public:
		serialize(T t_ = T{}) :
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


	/**
	* @class concurrent
	* Concurrent class from Herb Sutter that wraps any other class of type T and serializes every
	* member function call, effectively making the wrapped class thread-safe. The wrapping is done
	* by type-erasure. The concurrent class uses a concurrent_queue and worker thread to give
	* asynchronous non-blocking behavior, returning a std::future for all calls.
	* @see http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
	* @tparam	T	class or data wrapped for thread safety
	*/
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
		atomic<bool> done = false;
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
	* safe manner. Invoked lambdas are internally synchronized using a mutex, so this blocks the
	* calling thread and can cause contention. This should almost never be used due to potential
	* over locking - (locking every method call is often too much), and under locking - (no
	* "transaction" level locking). Prefer the concurrent<T> class for an asynchronous API.
	* @see http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
	* @tparam	T	class or data wrapped for thread safety
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