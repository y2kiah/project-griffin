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
#include <istream>
#include <type_traits>
#include <cstdint>

namespace griffin {

	// std::vector

	/**
	* std::vector serialization, trivially-copyable type, use one large memory write
	*/
	template <typename T, typename std::enable_if<std::is_trivially_copyable<T>::value>::type* = nullptr>
	void serialize(std::ostream& out, const std::vector<T>& vec)
	{
		// write size
		std::vector<T>::size_type size = vec.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(size));
		// write data
		out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
	}

	/**
	* std::vector serialization, non-trivially-copyable type, iterate and serialize
	*/
	template <typename T, typename std::enable_if<!std::is_trivially_copyable<T>::value>::type* = nullptr>
	void serialize(std::ostream& out, const std::vector<T>& vec)
	{
		// write size
		std::vector<T>::size_type size = vec.size();
		out.write(reinterpret_cast<const char*>(&size), sizeof(size));
		// write data
		for (T& i : vec) {
			griffin::serialize(out, i);
		}
	}

	/**
	* std::vector deserialization, trivially-copyable type, use one large memory read
	*/
	template <typename T, typename std::enable_if<std::is_trivially_copyable<T>::value>::type* = nullptr>
	void deserialize(std::istream& in, std::vector<T>& vec)
	{
		std::vector<T>::size_type size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		vec.resize(size);
		in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
	}

	/**
	* std::vector deserialization, non-trivially-copyable type, iterate and deserialize
	*/
	template <typename T, typename std::enable_if<!std::is_trivially_copyable<T>::value>::type* = nullptr>
	void deserialize(std::istream& in, std::vector<T>& vec)
	{
		std::vector<T>::size_type size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		vec.resize(size);
		for (int i = 0; i < size; ++i) {
			griffin::deserialize(in, &vec[i]);
		}
	}


	// griffin::handle_map

	/**
	* griffin::handle_map serialization
	*/
	template <typename T>
	void handle_map<T>::serialize(std::ostream& out, const handle_map<T>& map)
	{
		out << "handle_map";
		out.write(reinterpret_cast<const char*>(&map.m_freeListFront), sizeof(map.m_freeListFront));
		out.write(reinterpret_cast<const char*>(&map.m_freeListBack), sizeof(map.m_freeListBack));
		out.write(reinterpret_cast<const char*>(&map.m_itemTypeId), sizeof(map.m_itemTypeId));

		// copy the sparse ids
		static_assert(std::is_trivially_copyable<IdSet_T::value_type>::value, "IdSet_T is not trivially copyable");
		griffin::serialize(out, map.m_sparseIds);

		// copy the items
		griffin::serialize(out, map.m_items);

		// copy the meta set
		static_assert(std::is_trivially_copyable<handle_map<T>::MetaSet_T::value_type>::value, "MetaSet_T is not trivially copyable");
		griffin::serialize(out, map.m_meta);
	}

	/**
	* griffin::handle_map deserialization
	*/
	template <typename T>
	void handle_map<T>::deserialize(std::istream& in, handle_map<T>& map)
	{
		char tag[11] = {};
		in.read(tag, 10);
		if (strcmp("handle_map", tag) != 0) {
			throw(std::runtime_error("deserialization error at handle_map"));
		}
		in.read(reinterpret_cast<char*>(&map.m_freeListFront), sizeof(map.m_freeListFront));
		in.read(reinterpret_cast<char*>(&map.m_freeListBack), sizeof(map.m_freeListBack));
		in.read(reinterpret_cast<char*>(&map.m_itemTypeId), sizeof(map.m_itemTypeId));
		griffin::deserialize(in, map.m_sparseIds);
		griffin::deserialize(in, map.m_items);
		griffin::deserialize(in, map.m_meta);
	}


	// griffin::entity::ComponentStore

	/**
	* griffin::entity::ComponentStore serialization
	* @tparam Component		component type or type that has used the REFLECT macro to create a
	*		Reflection class for itself
	*/
	template <typename Component>
	void serialize(std::ostream& out, const entity::ComponentStore<Component>& store)
	{
		out << "ComponentStore";
		handle_map<typename entity::ComponentStore<Component>::ComponentRecord>::serialize(out, store.getComponents());
	}

	/**
	* griffin::entity::ComponentStore deserialization
	* @tparam Component		component type or type that has used the REFLECT macro to create a
	*		Reflection class for itself
	*/
	template <typename Component>
	void deserialize(std::istream& in, entity::ComponentStore<Component>& store)
	{
		char tag[15] = {};
		in.read(tag, 14);
		if (strcmp("ComponentStore", tag) != 0) {
			throw(std::runtime_error("deserialization error at ComponentStore"));
		}
		handle_map<typename entity::ComponentStore<Component>::ComponentRecord>::deserialize(in, store.getComponents());
	}


	// griffin::entity::ComponentStore::ComponentRecord

	/**
	* griffin::entity::ComponentStore::ComponentRecord serialization
	* @tparam Component		component type or type that has used the REFLECT macro to create a
	*		Reflection class for itself
	*/
	template <typename Component>
	void serialize(std::ostream& out, const typename entity::ComponentStore<Component>::ComponentRecord& rec)
	{
		griffin::serialize(out, rec.component);
		out.write(reinterpret_cast<const char*>(&rec.entityId.value), sizeof(rec.entityId.value));
	}

	/**
	* griffin::entity::ComponentStore::ComponentRecord deserialization
	* @tparam Component		component type or type that has used the REFLECT macro to create a
	*		Reflection class for itself
	*/
	template <typename Component>
	void deserialize(std::istream& in, typename entity::ComponentStore<Component>::ComponentRecord& rec)
	{
		griffin::deserialize(in, rec.component);
		in.read(reinterpret_cast<char*>(&rec.entityId.value), sizeof(rec.entityId.value));
	}


	// griffin::entity Component types

	/**
	* @tparam Component		component type or type that has used the REFLECT macro to create a
	*		Reflection class for itself
	*/
	template <typename Component>
	void serialize_component(std::ostream& out, Component& obj) {
		auto& props = Component::Reflection::getProperties();

		for (int p = 0; p < props.size(); ++p) {
			auto& prop = props[p];
			const char* valueBytes = reinterpret_cast<const char*>(&obj + prop.offset);

			if (prop.isTriviallyCopyable) {
				out.write(valueBytes, prop.size);
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

#endif