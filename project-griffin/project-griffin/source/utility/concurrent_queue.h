#pragma once
#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <mutex>
#include <list>

// finish this, look at both Sean Parent's and Herb Sutter's concurrent_queue implementations

template<typename T>
class concurrent_queue {
private:
	std::mutex   mutex;
	std::list<T> queue;

public:
	void enqueue(T x) {
		list<T> tmp;
		tmp.push_back(std::move(x));
		{
			std::lock_guard<std::mutex> lock(mutex);
			queue.splice(std::end(queue), tmp);
		}
	}
};

//#pragma once
//
//#include <queue>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread/condition_variable.hpp>
//
//using boost::mutex;
//using boost::condition_variable;
//
///**
//* class ConcurrentQueue
//* ConcurrentQueue was written by Anthony Williams. For an explanation of how
//* it works, go to:
//* http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
//*/
//template<typename T>
//class ConcurrentQueue {
//private:
//	std::queue<T>		mQueue;
//	mutable mutex		mMutex;
//	condition_variable	mCondVar;
//
//public:
//	/**
//	* Thread-safe push onto the queue. Also updates the condition
//	* variable so any threads locked in waitPop will take the mutex and
//	* process the pop.
//	*/
//	void push(const T &inData)
//	{
//		mutex::scoped_lock lock(mMutex);
//		mQueue.push(inData);
//		lock.unlock();
//		mCondVar.notify_one();
//	}
//
//	/**
//	* Thread-safe check if the queue is empty
//	*/
//	bool empty() const
//	{
//		mutex::scoped_lock lock(mMutex);
//		return mQueue.empty();
//	}
//
//	/**
//	* This pops an item from the queue, and if the list is empty, returns
//	* immediately instead of waiting for an item. Most likely would use
//	* this in the main thread to pop items pushed from a worker thread.
//	*/
//	bool tryPop(T &outData)
//	{
//		mutex::scoped_lock lock(mMutex);
//		if (mQueue.empty()) {
//			return false;
//		}
//
//		outData = mQueue.front();
//		mQueue.pop();
//		return true;
//	}
//
//	/**
//	* This waits on a condition variable for data to be available in the
//	* queue to pop. Most likely would use this in a worker thread to pop
//	* items that are pushed from the main thread.
//	*/
//	void waitPop(T &outData)
//	{
//		mutex::scoped_lock lock(mMutex);
//		while (mQueue.empty()) {
//			mCondVar.wait(lock);
//		}
//
//		outData = mQueue.front();
//		mQueue.pop();
//	}
//};

#endif