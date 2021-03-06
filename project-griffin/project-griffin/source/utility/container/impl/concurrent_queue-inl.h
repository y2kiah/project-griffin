/**
 * @file	concurrent_queue-inl.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_CONCURRENT_QUEUE_INL_H_
#define GRIFFIN_CONCURRENT_QUEUE_INL_H_

#include "../concurrent_queue.h"

namespace griffin {

	template <typename T>
	inline void concurrent_queue<T>::push(const T& inData)
	{
		std::unique_lock<mutex> lock(m_mutex);
		m_queue.push(inData);
		lock.unlock();
		m_cond.notify_one();
	}


	template <typename T>
	void concurrent_queue<T>::push(T&& inData)
	{
		std::unique_lock<mutex> lock(m_mutex);
		m_queue.push(std::forward<T>(inData));
		lock.unlock();
		m_cond.notify_one();
	}


	template <typename T>
	template <template <class T, class = std::allocator<T>> class Cnt>
	void concurrent_queue<T>::push_all(const Cnt<T>& inData)
	{
		std::unique_lock<mutex> lock(m_mutex);

		m_queue.reserve(m_queue.size() + inData.size());
		std::copy(inData.begin(), inData.end(), std::back_inserter(m_queue));

		lock.unlock();
		m_cond.notify_one();
	}


	template <typename T>
	template <template <class T, class = std::allocator<T>> class Cnt>
	void concurrent_queue<T>::push_all_move(Cnt<T>&& inData)
	{
		std::unique_lock<mutex> lock(m_mutex);
		
		m_queue.reserve(m_queue.size() + inData.size());
		std::move(inData.begin(), inData.end(), std::back_inserter(m_queue));
		
		lock.unlock();
		m_cond.notify_one();
		inData.clear();
	}


	template <typename T>
	bool concurrent_queue<T>::try_pop(T& outData)
	{
		std::lock_guard<mutex> lock(m_mutex);
		if (m_queue.empty()) {
			return false;
		}

		outData = std::move(m_queue.front());
		m_queue.pop();

		return true;
	}


	template <typename T>
	bool concurrent_queue<T>::try_pop(T& outData, const std::chrono::milliseconds& timeout)
	{
		std::unique_lock<mutex> lock(m_mutex);
		if (!m_cond.wait_for(lock, timeout, [this]{ return !m_queue.empty(); })) {
			return false;
		}

		outData = std::move(m_queue.front());
		m_queue.pop();

		return true;
	}


	template <typename T>
	template <template <class T, class = std::allocator<T>> class Cnt>
	int concurrent_queue<T>::try_pop_all(Cnt<T>& outData, int max)
	{
		int numPopped = 0;

		std::lock_guard<mutex> lock(m_mutex);

		while (!m_queue.empty() &&
			(max <= 0 || numPopped < max))
		{
			outData.emplace_back(std::move(m_queue.front()));
			m_queue.pop();

			++numPopped;
		}
		return numPopped;
	}


	template <typename T>
	template <class UnaryPredicate>
	bool concurrent_queue<T>::try_pop_if(T& outData, UnaryPredicate p_)
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


	template <typename T>
	template <template <class T, class = std::allocator<T>> class Cnt, class UnaryPredicate>
	int concurrent_queue<T>::try_pop_all_if(Cnt<T>& outData, UnaryPredicate p_)
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


	template <typename T>
	void concurrent_queue<T>::wait_pop(T& outData)
	{
		std::unique_lock<mutex> lock(m_mutex);
		m_cond.wait(lock, [this]() { return !m_queue.empty(); });

		outData = std::move(m_queue.front());
		m_queue.pop();
	}


	template <typename T>
	T concurrent_queue<T>::wait_pop()
	{
		std::unique_lock<mutex> lock(m_mutex);
		m_cond.wait(lock, [this]() { return !m_queue.empty(); });

		T outData = std::move(m_queue.front());
		m_queue.pop();

		return std::move(outData);
	}


	template <typename T>
	void concurrent_queue<T>::clear()
	{
		std::lock_guard<mutex> lock(m_mutex);
		m_queue.clear();
	}


	template <typename T>
	inline bool concurrent_queue<T>::empty() const
	{
		std::lock_guard<mutex> lock(m_mutex);
		return m_queue.empty();
	}


	template <typename T>
	inline size_t concurrent_queue<T>::unsafe_size() const
	{
		return m_queue.size();
	}


	template <typename T>
	inline size_t concurrent_queue<T>::unsafe_capacity() const
	{
		return m_queue.capacity();
	}
}

#endif