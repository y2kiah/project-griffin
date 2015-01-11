/**
 * @file	concurrent_queue.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_CONCURRENT_QUEUE_H
#define GRIFFIN_CONCURRENT_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <utility/container/vector_queue.h>

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
		void push(const T& inData);

		void push(T&& inData);

		/**
		 * Copies all items in container inData into the queue.
		 * @tparam	Cnt		Container of type T, such as vector, list, deque.
		 * @param	inData	container of items to be moved into queue
		 */
		template <template <class T, class = std::allocator<T>> class Cnt>
		void push_all(const Cnt<T>& inData);

		/**
		 * Moves all items in container inData into the queue. Clears inData.
		 * @tparam	Cnt		Container of type T, such as vector, list, deque.
		 * @param	inData	container of items to be moved into queue
		 */
		template <template <class T, class=std::allocator<T>> class Cnt>
		void push_all_move(Cnt<T>&& inData);

		/**
		 * Pops an item from the queue, or returns immediately without waiting if the list is
		 * empty. Most likely would use this in the main thread to pop items pushed from a worker
		 * thread.
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @returns true if pop succeeds, false if queue is empty
		 */
		bool try_pop(T& outData);

		/**
		 * Pops an item from the queue, or waits for specified timeout period for an item.
		 * Most likely would use this in the main thread to pop items pushed from a worker thread.
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @param	timeout    timeout period in milliseconds
		 * @returns true if pop succeeds, false if queue is empty for duration
		 */
		bool try_pop(T& outData, const std::chrono::milliseconds& timeout);

		/**
		* Pops several items from the queue, or returns immediately without waiting if the list is
		* empty. Items are popped only up to a max count passed in.
		* @tparam	Cnt		Container that implements emplace_back, such as vector, list, deque.
		* @param	outData	The popped items are emplaced into the provided container.
		* @param	max		maximum number of items to pop, or 0 (default) for unlimited
		* @returns number of items popped (and emplaced in outData)
		*/
		template <template <class T, class = std::allocator<T>> class Cnt>
		int try_pop_all(Cnt<T>& outData, int max = 0);

		/**
		 * Pops an item from the queue, or returns immediately without waiting if the list is empty
		 * only if the provided predicate function evaluates to true.
		 * @tparam	UnaryPredicate	predicate must return bool and accept a single param of type T
		 * @param	outData	   memory location to move item into, only modified if true is returned
		 * @param	p_		   function pointer, lambda or functor of type UnaryPredicate
		 * @returns true if pop succeeds, false if queue is empty
		 */
		template <class UnaryPredicate>
		bool try_pop_if(T& outData, UnaryPredicate p_);

		/**
		 * Pops several items from the queue, or returns immediately without waiting if the list is
		 * empty. Items are popped only while the provided predicate function evaluates to true.
		 * @tparam	Cnt		Container that implements emplace_back, such as vector, list, deque.
		 * @tparam	UnaryPredicate	predicate must return bool and accept a single param of type T
		 * @param	outData	The popped items are emplaced into the provided container.
		 * @param	p_		function pointer, lambda or functor of type UnaryPredicate
		 * @returns number of items popped (and emplaced in outData)
		 */
		template <template <class T, class=std::allocator<T>> class Cnt, class UnaryPredicate>
		int try_pop_all_if(Cnt<T>& outData, UnaryPredicate p_);

		/**
		 * Waits indefinitely for a condition variable that indicates data is available in the
		 * queue. Most likely would use this in a worker thread to execute tasks pushed from a
		 * client thread.
		 * @param	outData	   memory location to move item into
		 */
		void wait_pop(T& outData);
		T wait_pop();

		/**
		 * Thread-safe check of whether the queue is empty
		 * @returns whether the queue is empty: i.e. whether its size is zero
		 */
		bool empty() const;

		/**
		 * unsafe_size is not concurrency-safe and can produce incorrect results if called
		 * concurrently with calls to push*, pop*, and empty methods.
		 * @returns size of the queue
		 */
		size_t unsafe_size() const;

	private:
		mutable mutex		m_mutex;
		condition_variable	m_cond;
		vector_queue<T>		m_queue;
	};

}

#include "impl/concurrent_queue.inl"

#endif