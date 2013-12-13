#pragma once
#ifndef _COMPONENTSTORE_H
#define _COMPONENTSTORE_H

template <typename T>
class ComponentStore {
public:
	size_t createComponent() {
		size_t index = 0;
		if (emptyList.empty()) {
			components.emplace_back();
			index = components.size() - 1;
		} else {
			index = emptyList.back();
			emptyList.pop_back();
		}
		return index;
	}

	inline size_t createComponent(T & component) {
		size_t index = createComponent();
		component = components[index];
		return index;
	}

	// Don't store the reference for longer than the duration of a single system's frame tick. It
	// would break the model of systems operating on entity component lists per frame. It would also
	// be unsafe because this is not a ref-counted pointer so it could result in reference to corrupt
	// memory if the entity is deleted by another process.
	inline T & getComponent(size_t index) {
		return components[index];
	}

	explicit ComponentStore(size_t reserveCount) {
		components.reserve(reserveCount);
	}

private:
	std::vector<T>		components;
	std::vector<size_t>	emptyList;
};

#endif