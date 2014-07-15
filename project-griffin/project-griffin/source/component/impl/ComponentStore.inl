/**
* @author	Jeff Kiah
*/
#pragma once
#ifndef COMPONENTSTORE_INL
#define COMPONENTSTORE_INL

#include "../ComponentStore.h"
#include <sstream>

// struct ComponentId

/**
* stream debug print for a ComponentId
*/
inline std::ostream& operator<<(std::ostream& os, ComponentId id) {
	return os << "\n{{ idx:" << id.index << ", gen:" << id.generation << ", ct:" << id.typeId <<
		", free:" << id.free << "}, val:" << id.value << "}";
}

// class ComponentStore 

/**
* to_string for debug and test output
*/
template <typename T>
std::string ComponentStore<T>::to_string() const {
	std::ostringstream oss;

	auto ids = components.getIds();
	auto cmps = components.getItems();
	auto meta = components.getMeta();

	// print componentIds array
	oss << "ComponentStore<" << T::Reflection::getClassType() << ">: {\ncomponentIds: [";
	for (int i = 0; i < std::min(10, static_cast<int>(ids.size())); ++i) {
		oss << ids[i];
	}
	if (ids.size() > 10) { oss << "\n... n=" << ids.size() << "\n"; }

	// print denseToSparse values
	oss << "],\ndenseToSparse: [ ";
	for (int c = 0; c < std::min(10, static_cast<int>(meta.size())); ++c) {
		oss << meta[c].m_denseToSparse << ", ";
	}
	if (cmps.size() > 10) { oss << "... n=" << cmps.size(); }
	oss << " ],\nfreeListFront: " << components.getFreeListFront() << ",\nfreeListBack: "
		<< components.getFreeListBack() << "\n}\n";

	return oss.str();
}

#endif