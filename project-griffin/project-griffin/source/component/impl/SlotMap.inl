/**
* @author	Jeff Kiah
*/
#pragma once
#ifndef SLOTMAP_INL
#define SLOTMAP_INL

#include "../SlotMap.h"
#include <cassert>
#include <algorithm>

// struct Id_T comparison free functions

inline bool operator==(Id_T a, Id_T b) { return (a.value == b.value); }
inline bool operator!=(Id_T a, Id_T b) { return (a.value != b.value); }
inline bool operator< (Id_T a, Id_T b) { return (a.value <  b.value); }
inline bool operator> (Id_T a, Id_T b) { return (a.value >  b.value); }

// class ContiguousSet 

/**
 * create n items with default initialization and return a vector of their ids
 */
template <typename T>
IdSet_T SlotMap<T>::createItems(int n)
{
	m_items.reserve(m_items.size() + n); // pre-reserve the space we need (if not already there)
	m_meta.reserve(m_meta.size() + n);

	IdSet_T outerIds(n);
	std::generate_n(outerIds.begin(), n, [this](){ return createItem(); });

	return outerIds; // efficient to return a vector by value due to NVRO or C++11 move semantics
}

/**
 * add one item, moving the provided i into the store, return id
 */
template <typename T>
Id_T SlotMap<T>::addItem(T&& i)
{
	ComponentId outerId = {0};

	if (freeListEmpty()) {
		Id_T innerId = {
			(uint32_t)m_items.size(),
			1,
			m_itemTypeId,
			0
		};

		outerId = innerId;
		outerId.index = (uint32_t)m_sparseIds.size();

		m_sparseIds.push_back(innerId);

	} else {
		uint32_t outerIndex = m_freeListFront;
		ComponentId &innerId = m_sparseIds.at(outerIndex);

		m_freeListFront = innerId.index; // the index of a free slot refers to the next free slot
		if (freeListEmpty()) {
			m_freeListBack = m_freeListFront;
		}

		// convert the index from freelist to inner index
		innerId.free = 0;
		innerId.index = (uint32_t)m_items.size();

		outerId = innerId;
		outerId.index = outerIndex;
	}

	m_items.push_back(std::forward<T>(i));
	m_meta.push_back({ outerId.index });

	return outerId;
}

/**
 * remove the item identified by the provided outerId
 */
template <typename T>
void SlotMap<T>::removeItem(Id_T outerId)
{
	Id_T innerId = m_sparseIds[outerId.index];
	uint32_t innerIndex = innerId.index;

	/* debug build asserts */
	assert(outerId.typeId == m_itemTypeId && "typeId mismatch with ContiguousSet");
	assert(outerId.generation == innerId.generation && "removeItem called with old generation");
	assert(outerId.index < m_sparseIds.size() && "outer index out of range");
	assert(innerIndex < m_items.size() && "inner index out of range");
	/**/

	// push this slot to the back of the freelist
	innerId.free = 1;
	++innerId.generation; // increment generation so remaining outer ids go stale
	innerId.index = 0xFFFFFFFF; // max numeric value represents the end of the freelist
	m_sparseIds[outerId.index] = innerId; // write outer id changes back to the array

	if (freeListEmpty()) {
		// if the freelist was empty, it now starts (and ends) at this index
		m_freeListFront = outerId.index;
		m_freeListBack = m_freeListFront;

	} else {
		m_sparseIds[m_freeListBack].index = outerId.index; // previous back of the freelist points to new back
		m_freeListBack = outerId.index; // new freelist back is stored
	}

	// remove the component by swapping with the last element, then pop_back
	if (items.size() > 1) {
		std::swap(m_items.at(innerIndex), m_items.back());
		std::swap(m_meta.at(innerIndex), m_meta.back());

		// fix the ComponentId index of the swapped component
		m_sparseIds[m_meta.at(innerIndex).m_denseToSparse].index = innerIndex;
	}

	// do I really need to do this? it could be faster to just decrement a size counter and leave the memory untouched (not run any potential destructor on the component object, if it has one)
	m_items.pop_back();
	m_meta.pop_back();
}

/**
 * Get a direct reference to a stored item by outerId
 */
template <typename T>
inline T& SlotMap<T>::getItem(Id_T outerId)
{
	Id_T innerId = m_sparseIds[outerId.index];

	/* debug build asserts */
	assert(outerId.typeId == m_itemTypeId && "typeId mismatch with ContiguousSet");
	assert(outerId.generation == innerId.generation && "getComponent called with old generation");
	assert(outerId.index < m_sparseIds.size() && "outer index out of range");
	assert(innerIndex < m_items.size() && "inner index out of range");
	/**/

	return m_items[innerId.index];
}

#endif