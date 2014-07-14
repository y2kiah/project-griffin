/**
* @author	Jeff Kiah
*/
#pragma once
#ifndef SLOTMAP_SET
#define SLOTMAP_SET

#include <cstdint>
#include <vector>

using std::vector;
using std::pair;

/**
* @free 0 if active, 1 if slot is part of freelist, only applicable to inner ids
* @typeId relates to Tid template parameter of ContiguousSet
* @generation incrementing generation of data at the index, for tracking accesses to old data
* @index When used as an outer id (given to the client):
*			if free==0, index of id in the sparseIds array
*			if free==1, index of next free slot in the array, forming an embedded linked list
*		 When used as an inner id (stored in sparseIds array):
*			index of the item in the dense items array
*/
struct Id_T {
	union {
		// the order of these sub elements is important for sorting prioritized by free, then
		// typeId, then generation, and last by index
		struct {
			uint32_t index;
			uint16_t generation;
			uint16_t typeId : 15;
			uint16_t free   : 1;
		};
		uint64_t value;
	};
};
typedef vector<Id_T> IdSet_T;

/**
*
*/
template <typename T>
class SlotMap {
public:
	// Typedefs
	struct Meta_T {
		uint32_t denseToSparse;
	};

	typedef vector<T> DenseSet_T;
	typedef vector<Meta_T> MetaSet_T;

	// Functions

	/**
	* accessors
	*/
	const DenseSet_T& getItems() const         { return m_items; }
	const MetaSet_T&  getMeta() const          { return m_meta; }
	const IdSet_T&    getIds() const           { return m_sparseIds; }
	uint32_t          getFreeListFront() const { return m_freeListFront; }
	uint32_t          getFreeListBack() const  { return m_freeListBack; }

	/**
	* create one item with default initialization
	*/
	inline Id_T createItem() {
		return addItem(T{});
	}

	/**
	* create n items with default initialization and return a vector of their ids
	*/
	IdSet_T createItems(int n);

	/**
	* add one item, moving the provided i into the store, return id
	*/
	Id_T addItem(T&& i);

	/**
	* remove the item identified by the provided outerId
	*/
	void removeItem(Id_T outerId);

	/**
	* Get a direct reference to a stored item by outerId
	*/
	inline T& getItem(Id_T outerId);

	/**
	* Constructor takes a reserveCount to initialize the inner storage
	*/
	explicit SlotMap(uint16_t itemTypeId, size_t reserveCount)
		: m_itemTypeId(itemTypeId)
	{
		// in the future look into use of SFINAE to allow the class to compile with non default
		// constructible types as well
		static_assert(std::is_default_constructible<T>::value, "SlotMap type is not default constructible");

		m_sparseIds.reserve(reserveCount);
		m_items.reserve(reserveCount);
		m_meta.reserve(reserveCount);
	}

private:

	/**
	* freeList is empty when the front is set to 32 bit max value (the back will match)
	*/
	inline bool freeListEmpty() const {
		return (m_freeListFront == 0xFFFFFFFF);
	}

	// Member Variables

	// The embedded freelist is FIFO, 'delete's push to the back while 'new's pop from the front.
	uint32_t	m_freeListFront = 0xFFFFFFFF; // start index in the embedded ComponentId freelist
	uint32_t	m_freeListBack  = 0xFFFFFFFF; // last index in the freelist

	uint16_t	m_itemTypeId;	// the Id_T::typeId to use for ids produced by this SlotMap<T>

	IdSet_T		m_sparseIds;	// stores a list of ids
	DenseSet_T	m_items;		// stores items of type T
	MetaSet_T	m_meta;			// stores Meta_T type for each item
};

#include "impl/SlotMap.inl"

#endif