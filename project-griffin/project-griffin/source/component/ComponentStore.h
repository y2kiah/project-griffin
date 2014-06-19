#pragma once
#ifndef _COMPONENTSTORE_H
#define _COMPONENTSTORE_H

using std::vector;

template <typename T>
class ComponentStore {
public:
	size_t addComponent(/*T& component*/) // does this need to be rvalue reference (T&&)? 
	{
		size_t index = 0;
		if (freeList.empty()) {
			components.emplace_back(/*component*/); // this requires copy/move constructor, I want move. Since components are PODs, move should be provided by default, but need to test this to be sure. Do I need to wrap with std::forward??
			index = components.size() - 1;
		} else {
			index = freeList.back();
			freeList.pop_back();
		}
		//components[index] = component; // ensure copy not happening here, I want move. Do I need to wrap with std::forward??
		// Can I even achieve move here? The component already exists, and I'm trying to replace it with a new one, constructed elsewhere.
		// Might need to rethink this whole thing. Should this object be a factory as well? How to handle initialization in a Component agnostic way?
		
		return index;
	}

	inline size_t removeComponent(size_t index)
	{
		// remove needs to add the index to the freeList, and either sort the freeList by descending order, or insertion sort in that order,
		// so that pop_back from the freeList returns the first available spot in the vector. Use algorithm to achieve this, either sort or rotate,
		// or some other similar thing.
		return 0;
	}

	// Don't store the reference for longer than the duration of a single system's frame tick. It
	// would break the model of systems operating on entity component lists per frame. It would also
	// be unsafe because this is not a ref-counted pointer so it could result in reference to corrupt
	// memory if the entity is deleted by another process.
	inline T& getComponent(size_t index)
	{
		return components[index];
	}

	explicit ComponentStore(size_t reserveCount)
	{
		components.reserve(reserveCount);
	}

private:
	vector<T>		components;
	vector<size_t>	freeList;
};

#endif