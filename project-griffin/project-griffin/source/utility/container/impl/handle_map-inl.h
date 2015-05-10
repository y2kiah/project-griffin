/**
 * @file	handle_map-inl.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_HANDLE_MAP_INL_H_
#define GRIFFIN_HANDLE_MAP_INL_H_

#include "../handle_map.h"
#include <cassert>
#include <algorithm>

namespace griffin {

	// struct Id_T comparison free functions

	inline bool operator==(Id_T a, Id_T b) { return (a.value == b.value); }
	inline bool operator!=(Id_T a, Id_T b) { return (a.value != b.value); }
	inline bool operator< (Id_T a, Id_T b) { return (a.value < b.value); }
	inline bool operator> (Id_T a, Id_T b) { return (a.value > b.value); }

	// class handle_map 

	template <typename T>
	Id_T handle_map<T>::insert(T&& i)
	{
		Id_T handle = { 0 };

		if (freeListEmpty()) {
			Id_T innerId = {
				(uint32_t)m_items.size(),
				1,
				m_itemTypeId,
				0
			};

			handle = innerId;
			handle.index = (uint32_t)m_sparseIds.size();

			m_sparseIds.push_back(innerId);
		}
		else {
			uint32_t outerIndex = m_freeListFront;
			Id_T &innerId = m_sparseIds.at(outerIndex);

			m_freeListFront = innerId.index; // the index of a free slot refers to the next free slot
			if (freeListEmpty()) {
				m_freeListBack = m_freeListFront;
			}

			// convert the index from freelist to inner index
			innerId.free = 0;
			innerId.index = (uint32_t)m_items.size();

			handle = innerId;
			handle.index = outerIndex;
		}

		m_items.push_back(std::forward<T>(i));
		m_meta.push_back({ handle.index });

		return handle;
	}


	template <typename T>
	Id_T handle_map<T>::insert(const T& i)
	{
		return insert(T{ i });
	}


	template <typename T>
	template <typename... Params>
	IdSet_T handle_map<T>::emplaceItems(int n, Params... args)
	{
		IdSet_T handles(n);

		m_items.reserve(m_items.size() + n); // pre-reserve the space we need (if not already there)
		m_meta.reserve(m_meta.size() + n);

		std::generate_n(handles.begin(), n, [&, this](){ return emplace(args...); });

		return handles; // efficient to return vector by value, copy elided with NVRO (or with C++11 move semantics)
	}


	template <typename T>
	size_t handle_map<T>::erase(Id_T handle)
	{
		if (!isValid(handle)) {
			return 0;
		}

		Id_T innerId = m_sparseIds[handle.index];
		uint32_t innerIndex = innerId.index;

		// push this slot to the back of the freelist
		innerId.free = 1;
		++innerId.generation; // increment generation so remaining outer ids go stale
		innerId.index = 0xFFFFFFFF; // max numeric value represents the end of the freelist
		m_sparseIds[handle.index] = innerId; // write outer id changes back to the array

		if (freeListEmpty()) {
			// if the freelist was empty, it now starts (and ends) at this index
			m_freeListFront = handle.index;
			m_freeListBack = m_freeListFront;
		}
		else {
			m_sparseIds[m_freeListBack].index = handle.index; // previous back of the freelist points to new back
			m_freeListBack = handle.index; // new freelist back is stored
		}

		// remove the component by swapping with the last element, then pop_back
		if (m_items.size() > 1) {
			std::swap(m_items.at(innerIndex), m_items.back());
			std::swap(m_meta.at(innerIndex), m_meta.back());

			// fix the ComponentId index of the swapped component
			m_sparseIds[m_meta.at(innerIndex).denseToSparse].index = innerIndex;
		}

		m_items.pop_back();
		m_meta.pop_back();

		return 1;
	}

	
	template <typename T>
	size_t handle_map<T>::eraseItems(const IdSet_T& handles)
	{
		size_t count = 0;
		for (auto h : handles) {
			count += erase(h);
		}
		return count;
	}

	
	template <typename T>
	inline T& handle_map<T>::at(Id_T handle)
	{
		assert(handle.index < m_sparseIds.size() && "outer index out of range");

		Id_T innerId = m_sparseIds[handle.index];

		assert(handle.typeId == m_itemTypeId && "typeId mismatch");
		assert(handle.generation == innerId.generation && "at called with old generation");
		assert(innerId.index < m_items.size() && "inner index out of range");
		
		return m_items[innerId.index];
	}


	template <typename T>
	inline const T& handle_map<T>::at(Id_T handle) const
	{
		assert(handle.index < m_sparseIds.size() && "outer index out of range");

		Id_T innerId = m_sparseIds[handle.index];

		assert(handle.typeId == m_itemTypeId && "typeId mismatch");
		assert(handle.generation == innerId.generation && "at called with old generation");
		assert(innerId.index < m_items.size() && "inner index out of range");

		return m_items[innerId.index];
	}


	template <typename T>
	inline bool handle_map<T>::isValid(Id_T handle) const
	{
		if (handle.index >= m_sparseIds.size()) {
			return false;
		}
		
		Id_T innerId = m_sparseIds[handle.index];
		
		return (innerId.index < m_items.size() &&
				handle.typeId == m_itemTypeId &&
				handle.generation == innerId.generation);
	}
}

#endif