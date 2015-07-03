/**
* @file ComponentStoreSerialization.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITY_COMPONENTSTORESERIALIZATION_H_
#define GRIFFIN_ENTITY_COMPONENTSTORESERIALIZATION_H_

#include <utility/container/handle_map.h>
#include "components.h"
#include "ComponentStore.h"
#include <ostream>
#include <type_traits>
#include <cstdint>

namespace griffin {
	namespace entity {

		/**
		* std::vector serialization, trivially-copyable type, use one large memory write
		*/
		template <typename T,
				  typename std::enable_if<std::is_trivially_copyable<T>::value>::type* = nullptr>
		void serialize(std::ostream& out, const std::vector<T>& vec)
		{
			out << vec.size();
			out.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(T));
		}

		/**
		* std::vector serialization, non-trivially-copyable type, iterate and serialize
		*/
		template <typename T,
				  typename std::enable_if<!std::is_trivially_copyable<T>::value>::type* = nullptr>
		void serialize(std::ostream& out, const std::vector<T>& vec)
		{
			out << vec.size();
			for (T& i : vec) {
				serialize(out, i);
			}
		}

		/**
		* griffin::handle_map serialization
		*/
		template <typename T>
		void serialize(std::ostream& out, const handle_map<T>& map)
		{
			out << "handle_map" << map.getFreeListFront() << map.getFreeListBack() << map.getItemTypeId();

			// copy the sparse ids
			static_assert(std::is_trivially_copyable<IdSet_T::value_type>::value, "IdSet_T is not trivially copyable");
			serialize(out, map.getIds());

			// copy the items
			serialize(out, map.getItems());

			// copy the meta set
			static_assert(std::is_trivially_copyable<handle_map<T>::MetaSet_T::value_type>::value, "MetaSet_T is not trivially copyable");
			serialize(out, map.getMeta());
		}

		/**
		* griffin::entity::ComponentStore serialization
		* @tparam Component		component type or type that has used the REFLECT macro to create a
		*		Reflection class for itself
		*/
		template <typename Component>
		void serialize(std::ostream& out, const griffin::entity::ComponentStore<Component>& store)
		{
			out << "ComponentStore";
			serialize(out, store.getComponents());
		}

		template <typename Component>
		void serialize(std::ostream& out, const typename griffin::entity::ComponentStore<Component>::ComponentRecord& rec)
		{
			serialize(out, rec.component);
			out << rec.entityId.value;
		}


		template <typename Component>
		void serialize_component(std::ostream& out, Component& obj) {
			auto vals = Component::Reflection::getAllValues(obj);
			auto& props = Component::Reflection::getProperties();

			for (int p = 0; p < props.size(); ++p) {
				auto& prop = props[p];

				if (prop.isTriviallyCopyable) {
					//out.write(reinterpret_cast<const char*>(thisVal), prop.size);
				}
				else if (prop.isArray) {
					for (int i = 0; i < prop.numElements; ++i) {
						//serialize(out, thisVal[i]);
					}
				}
				else {
					//serialize(out, thisVal);
				}
			}
		}
	}
}

#endif