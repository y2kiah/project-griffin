/**
 * @file	SlotMap.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef SLOTMAP_H
#define SLOTMAP_H

#include <cstdint>
#include <vector>

using std::vector;

/**
 * @struct Id_T
 * @var	free		0 if active, 1 if slot is part of freelist, only applicable to inner ids
 * @var	typeId		relates to Tid template parameter of ContiguousSet
 * @var	generation	incrementing generation of data at the index, for tracking accesses to old data
 * @var	index		When used as an outer id (given to the client):
 *						free==0, index of id in the sparseIds array
 *						free==1, index of next free slot, forming an embedded linked list
 *					When used as an inner id (stored in sparseIds array):
 *						index of the item in the dense items array
 * @var	value		unioned with the above four vars, used for direct comparison of ids
 */
struct Id_T {
	union {
		/**
		 * the order of this bitfield is important for sorting prioritized by free, then typeId,
		 * then generation, then index
		 */
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
 * @class SlotMap
 *	Stores objects using a dense inner array and sparse outer array scheme for good cache coherence
 *	of the inner items. The sparse array contains outer ids (handles) used to identify the item,
 *	and provides an extra indirection allowing the inner array to move items in memory to keep them
 *	tightly packed. The sparse array contains an embedded FIFO freelist, where removed ids push to
 *	the back while new ids pop from the front.
 *
 * @tparam	T	type of item to be stored
 */
template <typename T>//, typename ForeignId_T*/> // use SFINAE to make ForeignId_T optional (pass nullptr)
class SlotMap {
public:
	/**
	 * @struct Meta_T
	 */
	struct Meta_T {
		uint32_t		m_denseToSparse;	//!< index into m_sparseIds array stored in m_meta
		//ForeignId_T		m_foreignId;
	};

	typedef vector<T> DenseSet_T;
	typedef vector<Meta_T> MetaSet_T;
	//typedef boost::flat_map<ForeignId_T, Id_T> ForeignIdMap_T;

	// Functions
	const DenseSet_T&	getItems() const			{ return m_items; }
	const MetaSet_T&	getMeta() const				{ return m_meta; }
	const IdSet_T&		getIds() const				{ return m_sparseIds; }
	uint32_t			getFreeListFront() const	{ return m_freeListFront; }
	uint32_t			getFreeListBack() const		{ return m_freeListBack; }

	/**
	 * create one item with default initialization
	 * @returns the id
	 */
	inline Id_T createItem(/*ForeignId_T foreignId*/) {
		return addItem(T{}/*, foreignId*/);
	}

	/**
	 * create n items with default initialization and return a vector of their ids
	 * @param[in]	n	number of items to create
	 * @returns a collection of ids
	 */
	IdSet_T createItems(int n/*, ForeignId_T foreignId*/);

	/**
	 * add one item, moving the provided i into the store, return id
	 * @param[in]	i	rvalue ref of of the object to move into inner storage
	 * @returns the id
	 */
	Id_T addItem(T&& i/*, ForeignId_T foreignId*/);

	/**
	 * remove the item identified by the provided outerId
	 * @param[in]	outerId		id of the item
	 */
	void removeItem(Id_T outerId);

	void removeItems(const IdSet_T& outerIds) {}
	//void removeItems(ForeignId_T foreignId) {}

	/**
	 * Get a direct reference to a stored item by outerId
	 * @param[in]	outerId		id of the item
	 * @returns reference to the item
	 */
	inline T& getItem(Id_T outerId);

	/**
	 * Constructor
	 * @param	itemTypeId		typeId used by the Id_T::typeId variable for this container
	 * @param	reserveCount	reserve space for inner storage
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
	* @returns true if empty
	*/
	inline bool freeListEmpty() const {
		return (m_freeListFront == 0xFFFFFFFF);
	}

	// Variables

	uint32_t	m_freeListFront = 0xFFFFFFFF; //!< start index in the embedded ComponentId freelist
	uint32_t	m_freeListBack  = 0xFFFFFFFF; //!< last index in the freelist

	uint16_t	m_itemTypeId;	//!< the Id_T::typeId to use for ids produced by this SlotMap<T>

	IdSet_T		m_sparseIds;	//!< stores a set of Id_T
	DenseSet_T	m_items;		//!< stores items of type T
	MetaSet_T	m_meta;			//!< stores Meta_T type for each item
};

#include "impl/SlotMap.inl"

#endif