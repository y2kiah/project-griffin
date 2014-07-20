/**
* @file	concurrent_queue.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_CONCURRENT_QUEUE_H
#define GRIFFIN_CONCURRENT_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

using std::queue;
using std::mutex;
using std::condition_variable;

namespace griffin {

	/**
	 * @class concurrent_queue
	 * concurrent_queue provides functionality for thread-safe enqueue and dequeue operations. This
	 * implementation is purposely similar to the MS PPL class (probably the eventual standard)
	 * except that the thread safe iterator is not provided, and a wait_pop method is added.
	 * @tparam T	type of object stored in the queue
	 * @see http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
	 * @see http://stackoverflow.com/questions/15278343/c11-thread-safe-queue
	 */
	template <typename T>
	class concurrent_queue {
	public:
		/**
		 * Thread-safe push onto the queue. Also updates the condition variable so any threads
		 * locked in waitPop will take the mutex and process the pop.
		 * @param	inData	item to be moved into queue
		 */
		void push(const T& inData)
		{
			T d = inData;
			push(std::move(d));
		}

		void push(T&& inData)
		{
			std::unique_lock<mutex> lock(m_mutex);
			m_queue.push(std::forward<T>(inData));
			lock.unlock();
			m_cond.notify_one();
		}

		/**
		 * Moves all items in container inData into the queue. Clears inData.
		 * @tparam	Cnt		Container of type T, such as vector, list, deque.
		 * @param	inData	container of items to be moved into queue
		 */
		template <template <class T, class=std::allocator<T>> class Cnt>
		void push_all(Cnt<T>&& inData)
		{
			std::unique_lock<mutex> lock(m_mutex);
			std::move(inData.begin(), inData.end(), m_queue.end());
			lock.unlock();
			m_cond.notify_one();
			inData.clear();
		}

		/**
		 * Pops an item from the queue, or returns immediately without waiting if the list is
		 * empty. Most likely would use this in the main thread to pop items pushed from a worker
		 * thread.
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @returns true if pop succeeds, false if queue is empty
		 */
		bool try_pop(T& outData)
		{
			std::lock_guard<mutex> lock(m_mutex);
			if (m_queue.empty()) {
				return false;
			}

			outData = std::move(m_queue.front());
			m_queue.pop();

			return true;
		}

		/**
		 * Pops an item from the queue, or waits for specified timeout period for an item.
		 * Most likely would use this in the main thread to pop items pushed from a worker thread.
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @param	timeout    timeout period in milliseconds
		 * @returns true if pop succeeds, false if queue is empty for duration
		 */
		bool try_pop(T& outData, const std::chrono::milliseconds& timeout)
		{
			std::unique_lock<mutex> lock(m_mutex);
			if (!m_cond.wait_for(lock, timeout, [this]{ return !m_queue.empty(); })) {
				return false;
			}

			outData = std::move(m_queue.front());
			m_queue.pop();

			return true;
		}
		

		/**
		 * Pops an item from the queue, or returns immediately without waiting if the list is empty
		 * only if the provided predicate function evaluates to true.
		 * @tparam	UnaryPredicate	predicate must return bool and accept a single param of type T
		 * @param	p_		   function pointer, lambda or functor of type UnaryPredicate
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @returns true if pop succeeds, false if queue is empty
		 */
		template <class UnaryPredicate>
		bool try_pop_if(T& outData, UnaryPredicate p_)
		{
			std::lock_guard<mutex> lock(m_mutex);
			if (!m_queue.empty() &&
				p_(m_queue.front()))
			{
				outData = std::move(m_queue.front());
				m_queue.pop();

				return true;
			}
			return false;
		}

		/**
		 * Pops several items from the queue, or returns immediately without waiting if the list is
		 * empty. Items are popped only while the provided predicate function evaluates to true.
		 * @tparam	Cnt		Container that implements emplace_back, such as vector, list, deque.
		 * @tparam	UnaryPredicate	predicate must return bool and accept a single param of type T
		 * @param	p_		function pointer, lambda or functor of type UnaryPredicate
		 * @param	outData	The popped items are emplaced into the provided container.
		 * @returns number of items popped (and emplaced in outData)
		 */
		template <template <class T, class=std::allocator<T>> class Cnt,
				  class UnaryPredicate>
		int try_pop_all_if(Cnt<T>& outData, UnaryPredicate p_)
		{
			int numPopped = 0;

			std::lock_guard<mutex> lock(m_mutex);

			while (!m_queue.empty() &&
				p_(m_queue.front()))
			{
				outData.emplace_back(std::move(m_queue.front()));
				m_queue.pop();

				++numPopped;
			}
			return numPopped;
		}

		/**
		 * Waits indefinitely for a condition variable that indicates data is available in the
		 * queue. Most likely would use this in a worker thread to execute tasks pushed from a
		 * client thread.
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 */
		void wait_pop(T& outData)
		{
			std::unique_lock<mutex> lock(m_mutex);
			m_cond.wait(lock, [this]() { return !m_queue.empty(); });

			outData = std::move(m_queue.front());
			m_queue.pop();
		}

		/**
		 * Thread-safe check of whether the queue is empty
		 * @returns whether the queue is empty: i.e. whether its size is zero
		 */
		bool empty() const
		{
			std::lock_guard<mutex> lock(m_mutex);
			return m_queue.empty();
		}

	private:
		mutable mutex		m_mutex;
		condition_variable	m_cond;
		queue<T>			m_queue;
	};

}

#endif