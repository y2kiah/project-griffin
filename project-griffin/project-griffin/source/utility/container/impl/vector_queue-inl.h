/**
 * @file	vector_queue-inl.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_VECTOR_QUEUE_INL_H_
#define GRIFFIN_VECTOR_QUEUE_INL_H_

#include "../vector_queue.h"

namespace griffin {

	// Inline Non-member Functions
	template <typename T>
	void swap(vector_queue<T>& x, vector_queue<T>& y) _NOEXCEPT //noexcept(noexcept(x.swap(y)))
	{
		using std::swap;
		swap(x.m_queue, y.m_queue);
		swap(x.m_offset, y.m_offset);
	}

	// Inline Member Functions

	template <typename T>
	vector_queue<T>::vector_queue() :
		m_queue{},
		m_offset{ 0 }
	{}


	template <typename T>
	inline bool vector_queue<T>::empty() const
	{
		return (size() == 0);
	}


	template <typename T>
	inline typename vector_queue<T>::size_type vector_queue<T>::size() const
	{
		return m_queue.size() - m_offset;
	}


	template <typename T>
	inline T& vector_queue<T>::front()
	{
		return m_queue[m_offset];
	}


	template <typename T>
	inline const T& vector_queue<T>::front() const
	{
		return m_queue[m_offset];
	}


	template <typename T>
	inline T& vector_queue<T>::back()
	{
		return m_queue.back();
	}


	template <typename T>
	inline const T& vector_queue<T>::back() const
	{
		return m_queue.back();
	}


	template <typename T>
	inline typename vector_queue<T>::iterator vector_queue<T>::begin() _NOEXCEPT
	{
		return (m_queue.begin() + m_offset);
	}
	

	template <typename T>
	inline typename vector_queue<T>::const_iterator vector_queue<T>::cbegin() const _NOEXCEPT
	{
		return (m_queue.cbegin() + m_offset);
	}

	
	template <typename T>
	inline typename vector_queue<T>::iterator vector_queue<T>::end() _NOEXCEPT
	{
		return m_queue.end();
	}

	
	template <typename T>
	inline typename vector_queue<T>::const_iterator vector_queue<T>::cend() const _NOEXCEPT
	{
		return m_queue.cend();
	}


	template <typename T>
	inline void vector_queue<T>::push(const T& val)
	{
		m_queue.push_back(val);
	}


	template <typename T>
	inline void vector_queue<T>::push(T&& val)
	{
		m_queue.push_back(std::forward<T>(val));
	}


	template <typename T>
	template <class... Args>
	inline void vector_queue<T>::emplace(Args&&... args)
	{
		m_queue.emplace_back(std::forward<Args>(args));
	}


	template <typename T>
	inline void vector_queue<T>::pop()
	{
		++m_offset;
		if (size() == 0) {
			clear();
		}
	}


	template <typename T>
	inline void vector_queue<T>::swap(vector_queue<T>& x) _NOEXCEPT
	{
		swap(*this, x);
	}


	template <typename T>
	inline typename vector_queue<T>::size_type vector_queue<T>::get_offset() const _NOEXCEPT
	{
		return m_offset;
	}


	/**
	* Force a reset of the offset to 0, moving queued values from the end of the vector to the
	* front. This function can be used to enforce a maximum size of the queue, but should only be
	* used occasionally, as it incurs a potentially large memory move.
	*/
	template <typename T>
	inline void vector_queue<T>::reset_offset()
	{
		if (m_offset != 0) {
			std::move(begin(), end(), m_queue.begin());
			m_queue.resize(size());
			m_offset = 0;
		}
	}


	template <typename T>
	inline T& vector_queue<T>::operator[](typename vector_queue<T>::size_type n)
	{
		return m_queue[m_offset + n];
	}


	template <typename T>
	inline const T& vector_queue<T>::operator[](typename vector_queue<T>::size_type n) const
	{
		return m_queue[m_offset + n];
	}


	template <typename T>
	inline T& vector_queue<T>::at(typename vector_queue<T>::size_type n)
	{
		if (size() <= n) {
			throw std::out_of_range("Invalid vector_queue<T> subscript.");
		}
		return (*this[n]);
	}


	template <typename T>
	inline const T& vector_queue<T>::at(typename vector_queue<T>::size_type n) const
	{
		if (size() <= n) {
			throw std::out_of_range("Invalid vector_queue<T> subscript.");
		}
		return (*this[n]);
	}


	template <typename T>
	inline void vector_queue<T>::push_back(const T& val)
	{
		m_queue.push_back(val);
	}


	template <typename T>
	inline void vector_queue<T>::push_back(T&& val)
	{
		m_queue.push_back(std::forward<T>(val));
	}


	template <typename T>
	inline void vector_queue<T>::clear() _NOEXCEPT
	{
		m_offset = 0;
		m_queue.clear();
	}


	template <typename T>
	inline void vector_queue<T>::reserve(typename vector_queue<T>::size_type n)
	{
		// m_offset added to n because semantics of reserve are that caller wants enough room to
		// push new items, and this data structure offsets the beginning element
		m_queue.reserve(n + m_offset);
	}


	template <typename T>
	inline typename vector_queue<T>::size_type vector_queue<T>::capacity() const _NOEXCEPT
	{
		return m_queue.capacity();
	}

	
	template <typename T>
	inline typename vector_queue<T>::size_type vector_queue<T>::max_size() const _NOEXCEPT
	{
		return m_queue.max_size();
	}


	template <typename T>
	inline void vector_queue<T>::shrink_to_fit()
	{
		m_queue.shrink_to_fit();
	}


	template <typename T>
	inline T* vector_queue<T>::data() _NOEXCEPT
	{
		return m_queue.data;
	}


	template <typename T>
	inline const T* vector_queue<T>::data() const _NOEXCEPT
	{
		return m_queue.data;
	}

}

#endif