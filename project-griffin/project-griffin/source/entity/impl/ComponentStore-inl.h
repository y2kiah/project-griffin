/**
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_COMPONENTSTORE_INL_H_
#define GRIFFIN_COMPONENTSTORE_INL_H_

#include "../ComponentStore.h"
#include <sstream>

namespace griffin {
	namespace entity {

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

			auto ids = m_components.getIds();
			auto cmps = m_components.getItems();
			auto meta = m_components.getMeta();

			// print componentIds array
			oss << "ComponentStore<" << T::Reflection::getClassType() << ">: {\ncomponentIds: [";
			for (int i = 0; i < std::min(10, static_cast<int>(ids.size())); ++i) {
				oss << ids[i];
			}
			if (ids.size() > 10) { oss << "\n... n=" << ids.size() << "\n"; }

			// print denseToSparse values
			oss << "],\ndenseToSparse: [ ";
			for (int c = 0; c < std::min(10, static_cast<int>(meta.size())); ++c) {
				oss << meta[c].denseToSparse << ", ";
			}
			if (cmps.size() > 10) { oss << "... n=" << cmps.size(); }
			oss << " ],\nfreeListFront: " << m_components.getFreeListFront() << ",\nfreeListBack: "
				<< m_components.getFreeListBack() << "\n}\n";

			return oss.str();
		}

	}
}

#endif