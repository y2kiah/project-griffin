#pragma once
#ifndef COMPONENTSTORE_H
#define COMPONENTSTORE_H

#include <utility/container/handle_map.h>
#include <string>

using std::string;

typedef griffin::Id_T    ComponentId;
typedef griffin::IdSet_T ComponentIdList;

inline std::ostream& operator<<(std::ostream& os, ComponentId id);


template <typename T>
class ComponentStore {
public:
	// Typedefs
	typedef griffin::handle_map<T> ComponentMap;

	// Functions

	/**
	* get a const ref to the components ContiguousSet, useful for Systems
	*/
	inline const ComponentMap& getComponents() const {
		return components;
	}

	/**
	* create one component with default zero-initialization
	*/
	inline ComponentId createComponent() {
		return components.emplace();
	}

	/**
	* create n components and return a vector of their ComponentIds
	*/
	inline ComponentIdList createComponents(int n) {
		return components.emplaceItems(n);
	}

	/**
	* add one component, moving the provided cmp into the store, return ComponentId
	*/
	inline ComponentId addComponent(T&& cmp) {
		return components.addItem(std::forward<T>(cmp));
	}

	/**
	* remove the component identified by the provided outerId
	*/
	inline void removeComponent(ComponentId outerId) {
		components.removeItem(outerId);
	}

	/**
	* Don't store the reference for longer than the duration of a single system's frame tick. It
	* would break the model of systems operating on queried entity lists on a per-frame basis. It
	* would also be unsafe to hold a reference to invalid memory, incase the store is deleted.
	*/
	inline T& getComponent(ComponentId outerId) {
		return components[outerId];
	}

	/**
	* to_string for debug and test output
	*/
	std::string to_string() const;

	/**
	* Constructor takes a reserveCount to initialize the inner storage, and the itemTypeId is
	* automatically set to T::componentType
	*/
	explicit ComponentStore(size_t reserveCount) :
		components(T::componentType, reserveCount)
	{}

	/**
	* Constructor takes a reserveCount to initialize the inner storage, and the itemTypeId is also
	* passed in. This overload can be used if the typeId is not known at compile time, such as for
	* components that are defined at runtime by schema files or scripting languages. The type T in
	* that case would likely be homogenous
	*/
	explicit ComponentStore(uint16_t typeId, size_t reserveCount) :
		components(typeId, reserveCount)
	{}

private:
	// Member Variables

	ComponentMap components;
};

#include "impl/ComponentStore.inl"

#endif