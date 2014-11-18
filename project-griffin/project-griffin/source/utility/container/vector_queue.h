/**
 * @file	vector_queue.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_VECTOR_QUEUE_H
#define GRIFFIN_VECTOR_QUEUE_H

#include <vector>

using std::vector;

namespace griffin {
	/**
	 * @class vector_queue
	 * vector_queue implements a queue on top of std::vector, unlike std::queue which uses deque by
	 * default, and can optionally use list.
	 * @tparam T	type of object stored in the queue
	 */
	template <typename T>
	class vector_queue {
	public:
		// Typedefs
		typedef typename vector<T>::size_type size_type;

		// Queue Functions
		bool empty() const;
		size_type size() const;
		
		T& front();
		const T& front() const;
		T& back();
		const T& back() const;

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

#include <utility/container/impl/vector_queue.inl>

#endif