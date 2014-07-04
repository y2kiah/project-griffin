#pragma once
#ifndef COMPONENTSTORE_H
#define COMPONENTSTORE_H

#include <algorithm>
#include <sstream>
#include <cstdint>

using std::vector;

/**
* @free 0 if active, 1 if component is part of freelist
* @componentType tells the dictionary which store to look in
* @generation incrementing generation of data at the index, for tracking accesses to old data
* @index When used as an outer id (given to the client):
*			if free==0, index of the value in the sparse idList; if free==1, index of the next
*			free slot in the array, forming an embedded forward linked list, aka freelist
*		 When used as an inner id (stored in sparse idList):
*			index of the component in the dense array
*/
struct ComponentId {
	union {
		struct {
			uint16_t free          : 1;
			uint16_t componentType : 15;
			uint16_t generation;
			uint32_t index;
		};
		uint64_t value;
	};
};

// use the concept idiom that Sean Parent presents here http://www.youtube.com/watch?v=qH6sSOr-yk8
// so ComponentStore does not inherit from ComponentStoreBase here, it gets wrapped into the ComponentDictionary
class ComponentStoreBase {
public:
	virtual ~ComponentStoreBase() {}
};

template <typename T>
class ComponentStore : public ComponentStoreBase {
public:
	// move this back to private if getComponents is removed
	typedef std::pair<uint32_t, T> Pair_T; // first = dense-to-sparse index, second = component

	inline ComponentId createComponent()
	{
		return addComponent(T{});
	}

	ComponentId addComponent(T&& cmp)
	{
		ComponentId outerId = { 0 };
		
		if (freeListEmpty()) {
			ComponentId innerId = { 0,
									T::componentType,
									1,
									(uint32_t)components.size() };

			outerId = innerId;
			outerId.index = (uint32_t)idList.size();

			idList.push_back(innerId);

		} else {
			uint32_t outerIndex = freeListFront;
			ComponentId &innerId = idList.at(outerIndex);

			freeListFront = innerId.index; // the index of a free slot refers to the next free slot
			if (freeListEmpty()) {
				freeListBack = freeListFront;
			}

			// convert the index from freelist to inner index
			innerId.free = 0;
			innerId.index = (uint32_t)components.size();

			outerId = innerId;
			outerId.index = outerIndex;
		}

		components.emplace_back(outerId.index, std::forward<T>(cmp));

		return outerId;
	}
	
	void removeComponent(ComponentId outerId)
	{
		ComponentId innerId = idList[outerId.index];
		uint32_t innerIndex = innerId.index;

		/* debug build asserts */
		assert(outerId.componentType == T::componentType && "ComponentId type mismatch with ComponentStore");
		assert(outerId.generation == innerId.generation && "removeComponent called on deleted component");
		assert(outerId.index < idList.size() && "outer index out of range");
		assert(innerIndex < components.size() && "inner index out of range");
		/**/

		// to make this thread safe, and prevent races, use a mutex or concurrent queue to protect
		// consider implementing two versions, one "thread safe" and one not, incase a particlar
		// component store is guaranteed to be accessed from one thread, don't pay the price of locking

		// push this slot to the back of the freelist
		innerId.free = 1;
		++innerId.generation; // increment generation so remaining outer ids go stale
		innerId.index = 0xFFFFFFFF; // max numeric value represents the end of the freelist
		idList[outerId.index] = innerId; // write outer id changes back to the array

		if (freeListEmpty()) {
			// if the freelist was empty, it now starts (and ends) at this index
			freeListFront = outerId.index;
			freeListBack = freeListFront;

		} else {
			idList[freeListBack].index = outerId.index; // previous back of the freelist points to new back
			freeListBack = outerId.index; // new freelist back is stored
		}

		// remove the component by swapping with the last element, then pop_back
		if (components.size() > 1) {
			std::swap(components.at(innerIndex), components.back());
			// fix the ComponentId index of the swapped component
			idList[components.at(innerIndex).first].index = innerIndex;
		}
		components.pop_back(); // do I really need to do this? it could be faster to just decrement a size counter and leave the memory untouched (not run any potential destructor on the component object, if it has one)
	}

	/**
	* Don't store the reference for longer than the duration of a single system's frame tick. It
	* would break the model of systems operating on queried entity lists on a per-frame basis. It
	* would also be unsafe to hold a reference to invalid memory, incase the store is deleted.
	*/
	inline T& getComponent(ComponentId outerId)
	{
		ComponentId innerId = idList[outerId.index];

		/* debug build asserts */
		assert(outerId.componentType == T::componentType && "ComponentId type mismatch with ComponentStore");
		assert(outerId.generation == innerId.generation && "getComponent called on deleted component");
		assert(outerId.index < idList.size() && "outer index out of range");
		assert(innerId.index < components.size() && "inner index out of range");
		/**/

		return components[innerId.index].second;
	}

	std::string to_string() const {
		std::ostringstream oss;
		
		oss << "freeListFront=" << freeListFront << ", freeListBack=" << freeListBack << "\n[\n";
		for (auto id : idList) {
			oss << "  {T=" << id.componentType << ",F=" << id.free << ",G=" << id.generation << ",I=" << id.index << "}\n";
		}
		oss << "]\n[";
		for (auto cp : components) {
			oss << cp.first << ",";
		}
		oss << "]\n\n";

		return oss.str();
	}

	const vector<Pair_T>& getComponents() const { return components; }

	explicit ComponentStore(size_t reserveCount)
	{
		components.reserve(reserveCount);
	}

private:
	

	// The embedded freelist is FIFO, 'delete's push to the back while 'new's pop from the front.
	uint32_t			freeListFront = 0xFFFFFFFF; // start index in the embedded ComponentId freelist
	uint32_t			freeListBack  = 0xFFFFFFFF; // last index in the freelist

	vector<ComponentId>	idList;		// stores a list of ids
	vector<Pair_T>		components;

	inline bool freeListEmpty() const { return (freeListFront == 0xFFFFFFFF); }

};

#endif