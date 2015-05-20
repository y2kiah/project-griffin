/**
 * @file	vector_queue.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_VECTOR_QUEUE_H_
#define GRIFFIN_VECTOR_QUEUE_H_

#include <vector>

using std::vector;

namespace griffin {
	/**
	 * @class vector_queue
	 * vector_queue implements a queue on top of std::vector, unlike std::queue which uses deque or
	 * list. This container continues to grow while items are pushed and the queue is not empty.
	 * When the queue empties, the offset is set back to zero. This container should be used when
	 * contiguous memory is important, and when the queue is often filled and emptied in cycles.
	 * @tparam T	type of object stored in the queue
	 */
	template <typename T>
	class vector_queue {
	public:
		// Typedefs
		typedef typename vector<T>::size_type size_type;
		typedef typename vector<T>::iterator iterator;
		typedef typename vector<T>::const_iterator const_iterator;

		// Queue Functions
		bool empty() const;
		size_type size() const;
		
		T& front();
		const T& front() const;
		T& back();
		const T& back() const;

		iterator begin() _NOEXCEPT;
		const_iterator cbegin() const _NOEXCEPT;
		iterator end() _NOEXCEPT;
		const_iterator cend() const _NOEXCEPT;

		void push(const T& val);
		void push(T&& val);
		template <class... Args> void emplace(Args&&... args);
		void pop();

		void swap(vector_queue<T>& x) _NOEXCEPT;

		// Vector Functions
		T& operator[](size_type n);
		const T& operator[](size_type n) const;
		T& at(size_type n);
		const T& at(size_type n) const;
		void push_back(const T& val);
		void push_back(T&& val);
		void clear() _NOEXCEPT;
		void reserve(size_type n);
		size_type capacity() const _NOEXCEPT;
		size_type max_size() const _NOEXCEPT;
		void shrink_to_fit();
		T* data() _NOEXCEPT;
		const T* data() const _NOEXCEPT;

		// Constructors
		explicit vector_queue<T>::vector_queue();

	private:
		// Variables
		size_type	m_offset;
		vector<T>	m_queue;
	};
}

#include "impl/vector_queue-inl.h"

#endif