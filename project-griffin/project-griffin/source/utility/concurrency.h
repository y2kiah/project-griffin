/**
 * @file	concurrency.h
 * @author	Jeff Kiah
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
#ifndef GRIFFIN_CONCURRENCY_H
#define GRIFFIN_CONCURRENCY_H

#include <thread>
#include <mutex>
#include <future>
#include <utility/container/concurrent_queue.h>

namespace griffin {

	/**
	 * @class concurrent
	 * The concurrent class wraps any class T and accepts lambdas that take a reference to the contained
	 * object for performing operations (via member-function calls presumably) in a thread safe manner.
	 * Lambdas are queued in a concurrent_queue and run by a private worker thread. This is the non-
	 * blocking asychronous way to wrap a class for thread safety, and is highly composable with async
	 * patterns. Since each instance of this object creates its own thread, don't abuse this pattern by
	 * creating lots of these objects. This should wrap a high level class with few instances.
	 * See the backgrounder class example below for an illustration.
	 * @tparam	T	type of class or data wrapped for concurrency
	 */
	template <typename T>
	class concurrent {
	public:
		concurrent(T t_ = T{}) :
			t{ t_ },
			worker{ [=]{ while (!done) q.wait_pop()(); } }
		{}

		~concurrent() {
			q.push([=]{ done = true; });
			worker.join();
		}

		/**
		 * Executes function pointer, functor or lambda on the worker thread where the task is
		 * internally serialized by the concurrent queue for thread safety.
		 * @tparam	F	function pointer, functor or lambda accepting one argument of type T
		 * @param	f	type F
		 * @returns std::future of type T
		 */
		template <typename F>
		auto operator()(F f) const -> std::future<decltype(f(t))> {
			auto p = std::make_shared<std::promise<decltype(f(t))>>();
			auto ret = p->get_future();

			q.push([=]{
				try {
					set_value(*p, f, t);
				} catch (...) {
					p->set_exception(current_exception());
				}
			});

			return ret;
		}

		/**
		 * Wrappers for p.set_value() so the function above compiles for both void and non-void.
		 */
		template <typename Fut, typename F, typename T>
		void set_value(std::promise<Fut>& p, F& f, T& t) {
			p.set_value(f(t));
		}

		template <typename F, typename T>
		void set_value(std::promise<void>& p, F& f, T& t) {
			f(t);
			p.set_value();
		}

	private:
		mutable T t;
		mutable concurrent_queue<std::function<void()>> q;
		bool done = false;
		std::thread worker;
	};


	/**
	 * @class monitor
	 * The concurrent class wraps any class T and accepts lambdas that take a reference to the contained
	 * object for performing operations (via member-function calls presumably) in a thread safe manner.
	 * Invoked lambdas are internally synchronized using a mutex, so this would potentially block client
	 * threads and cause contention. This should almost never be used, prefer the concurrent<T> class
	 * and task parallelism when possible. This can used on lower level objects than concurrent<T> since
	 * we don't pay the price of one thread per object, only a mutex.
	 * @tparam	T	class or data wrapped to be protected by a mutex for thread safety
	 */
	template <typename T>
	class monitor {
	public:
		monitor(T t_ = T{}) : t(t_) {}

		template <typename F>
		auto operator()(F f) const -> decltype(f(t)) {
			std::lock_guard<std::mutex> _{ m };
			return f(t);
		}

		/*template <typename F>
		auto operator()(F f) const -> typename std::result_of<F(T)>::type {
			std::lock_guard<std::mutex> _{ m };
			return f(t);
		}*/

	private:
		mutable T t;
		mutable std::mutex m;
	};


	/**
	 * Example usage of concurrent<T>.
	 * @code
	 *	concurrent<string> s;
	 *	auto f = s([](string& s) {		// C++14 polymorphic lambda becomes s([](s) {
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

/*
	// The following classes may be built for cross platform compiling based on use of PPL

	class thread_pool {
	public:
		explicit thread_pool() {}

	private:
		vector<std::thread> m_threads;
	};


	template <typename F>
	class task {
	public:
		task(F&& f_) : f{ f_ } {}

	private:
		F f;

	};


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
}
#endif