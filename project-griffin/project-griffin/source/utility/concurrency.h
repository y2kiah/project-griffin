/**
* Two neat classes from Herb Sutter that wrap any other class of type T and synchronizes every
* member function call, effectively making the wrapped class thread-safe. The wrapping pattern used
* is interesting by itself. The concurrent class uses a concurrent_queue and worker thread to get
* asynchronous non-blocking behavior, returning a std::future. The monitor class uses a simple for
* thread safety, but should hardly ever be used because it both over-locks (every method is too
* much) and under-locks (no transaction-level locking) in most real situations. It can maybe be
* used for printf or cout logging, but even then a concurrent_queue is preferrable. The wrapping
* pattern can also be used in lieu of inheritance in many situations.
* http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
*/

#pragma once
#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include <thread>
#include <mutex>
#include <future>
#include "concurrent_queue.h"

/**
* The concurrent class wraps any class T and accepts lambdas that take a reference to the contained
* object for performing operations (via member-function calls presumably) in a thread safe manner.
* Lambdas are queued in a concurrent_queue and run by a private worker thread. This is the non-
* blocking asychronous way to wrap a class for thread safety, and is highly composable with async
* patterns. Since each instance of this object creates its own thread, don't abuse this pattern by
* creating lots of these objects. This should wrap a high level class with few instances.
* See the backgrounder class example below for an illustration.
* Example:
*	concurrent<string> s;
*	auto f = s([](string& s) {		// C++14 polymorphic lambda becomes s([](s) {
*		// thread safe transaction!
*		s += "added thread-safe";	// this is operating on the internal (wrapped) object
*		return string("this was ") + s;
*	});
*	// f is a std::future for the lambda return value
*/
template<class T>
class concurrent {
private:
	mutable T t;
	mutable concurrent_queue<std::function<void()>> q;
	bool done = false;
	std::thread worker;

public:
	concurrent(T t_) :
		t{ t_ },
		worker{ [=]{ while (!done) q.pop()(); } }
	{}
	~concurrent() {
		q.push([=]{ done = true; });
		worker.join();
	}

	template<typename F>
	auto operator()(F f) const -> std::future<decltype(f(t))> {
		auto p = make_shared<std::promise<decltype(f(t))>>();
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

	// wrappers for p.set_value() so the function above compiles for both void and non-void
	template<typename Fut, typename F, typename T>
	void set_value(std::promise<Fut>& p, F& f, T& t) {
		p.set_value(f(t));
	}
	
	template<typename F, typename T>
	void set_value(std::promise<void>& p, F& f, T& t) {
		f(t);
		p.set_value();
	}
};


/**
* The concurrent class wraps any class T and accepts lambdas that take a reference to the contained
* object for performing operations (via member-function calls presumably) in a thread safe manner.
* Invoked lambdas are internally synchronized using a mutex, so this would potentially block client
* threads and cause contention. This should almost never be used, prefer the concurrent<T> class
* and task parallelism when possible. This can used on lower level objects than concurrent<T> since
* we don't pay the price of one thread per object, only a mutex.
* Example:
*	monitor<string> s;
*	auto f = s([](string& s) {		// C++14 polymorphic lambda becomes s([](s) {
*		// thread safe transaction!
*		s += "added thread-safe";	// this is operating on the internal (wrapped) object
*		return string("this was ") + s;
*	});
*	// f is a string (note that with concurrent<t> this would be a future)
*/
template<class T>
class monitor {
private:
	mutable T t;
	mutable std::mutex m;

public:
	monitor(T t_ = T{}) : t(t_) {}

	template<typename F>
	auto operator() (F f) const -> decltype(f(t)) {
		std::lock_guard<std::mutex> _{ m };
		return f(t);
	}
};


/**
* One way
* to use the concurrent<t> class is to protect a private data member of an outer class (call it
* backgrounder for example) that exposes a simple public API. That way the weird usage pattern of
* submitting lambdas can be hidden within the implementation of the public APIs, and client code
* will not have to know or care about the concurrent<t> usage pattern.

class backgrounder {
public:
	std::future<bool> save(std::string file) {
		c([=](data& d) {
			// each function is an ordered transaction
		});
	}

	std::future<size_t> print(some& stuff) {
		c([=, &stuff](data& d) {
			// atomic and indivisible w.r.t. other functions
		});
	}

private:
	struct data {  };	// private data is unshared by construction
	concurrent<data> c;	// fully thread-safe internal data
};
*/


// multiple clients may read simultaneously
// but write access is exclusive
// writers are favoured over readers
/*class ReadWriteMutex {
public:
	ReadWriteMutex() :
		m_readers(0),
		m_pendingWriters(0),
		m_currentWriter(false)
	{}

	class ScopedReadLock {
	public:
		ScopedReadLock(ReadWriteMutex& rwLock) :
			m_rwLock(rwLock)
		{
			m_rwLock.acquireReadLock();
		}

		~ScopedReadLock() {
			m_rwLock.releaseReadLock();
		}

	private:
		ReadWriteMutex& m_rwLock;
	};

	class ScopedWriteLock {
	public:
		ScopedWriteLock(ReadWriteMutex& rwLock) :
			m_rwLock(rwLock)
		{
			m_rwLock.acquireWriteLock();
		}

		~ScopedWriteLock() {
			m_rwLock.releaseWriteLock();
		}

	private:
		ReadWriteMutex& m_rwLock;
	};


private: // data
	std::mutex m_mutex;

	unsigned int m_readers;
	std::condition m_noReaders;

	unsigned int m_pendingWriters;
	bool m_currentWriter;
	std::condition m_writerFinished;


private:

	void acquireReadLock() {
		std::mutex::scoped_lock lock(m_mutex);

		// require a while loop here, since when the writerFinished condition is notified
		// we should not allow readers to lock if there is a writer waiting
		// if there is a writer waiting, we continue waiting
		while (m_pendingWriters != 0 || m_currentWriter) {
			m_writerFinished.wait(lock);
		}
		++m_readers;
	}

	void releaseReadLock() {
		std::mutex::scoped_lock lock(m_mutex);
		--m_readers;

		if (m_readers == 0) {
			// must notify_all here, since if there are multiple waiting writers
			// they should all be woken (they continue to acquire the lock exclusively though)
			m_noReaders.notify_all();
		}
	}

	// this function is currently not exception-safe:
	// if the wait calls throw, m_pendingWriter can be left in an inconsistent state
	void acquireWriteLock() {
		std::mutex::scoped_lock lock(m_mutex);

		// ensure subsequent readers block
		++m_pendingWriters;

		// ensure all reader locks are released
		while (m_readers > 0) {
			m_noReaders.wait(lock);
		}

		// only continue when the current writer has finished 
		// and another writer has not been woken first
		while (m_currentWriter) {
			m_writerFinished.wait(lock);
		}
		--m_pendingWriters;
		m_currentWriter = true;
	}

	void releaseWriteLock() {
		std::mutex::scoped_lock lock(m_mutex);
		m_currentWriter = false;
		m_writerFinished.notify_all();
	}
};*/

#endif