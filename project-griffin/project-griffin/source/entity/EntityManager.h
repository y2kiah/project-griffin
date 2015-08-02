/**
* @file EntityManager.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITYMANAGER_H_
#define GRIFFIN_ENTITYMANAGER_H_

#include <boost/container/flat_map.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility/memory_reserve.h>
#include "ComponentStore.h"
#include "Entity.h"


namespace griffin {
	namespace entity {


		class EntityManager {
		public:
			// Typedefs
			typedef griffin::handle_map<Entity> EntityMap;
			typedef boost::container::flat_multimap<uint64_t, EntityId> ComponentMaskMap;
			typedef std::array<std::shared_ptr<ComponentStoreBase>, MAX_COMPONENTS> ComponentStoreMap;

			// Functions

			explicit EntityManager() :
				m_entityStore(0, RESERVE_ENTITYMANAGER_ENTITIES),
				m_componentStores{} // zero-init fills with nullptr, component store created on first access
			{}

			// Entity Functions

			/**
			* @return	true if entity id is valid
			*/
			bool entityIsValid(EntityId entityId) const {
				return m_entityStore.isValid(entityId);
			}

			/**
			* Creates a new empty entity
			* @return	entity id
			*/
			EntityId createEntity();

			/**
			* @return	ComponentId of a component with a certain type. Returns the first component
			*	in the list encountered. Use the getEntityComponents function if more than one
			*	component of a type is expected.
			*/
			ComponentId getEntityComponentId(EntityId entityId, ComponentType ct) const {
				if (m_entityStore.isValid(entityId)) {
					auto& entity = m_entityStore[entityId];

					for (auto cmp : entity.components) {
						if (cmp.typeId == ct) {
							return cmp;
						}
					}
				}
				return NullId_T;
			}

			/**
			* Get the ComponentIds that belong to an entity
			* @return	const reference to the components vector, don't store it just use it
			*	immediately and discard
			*/
			const std::vector<ComponentId>& getEntityComponents(EntityId entityId) const {
				return m_entityStore[entityId].components;
			}

			/**
			* Adds a component to an existing entity
			* @param entityId	entity to receive the new component
			* @tparam T	the component type, requires member T::componentType
			* @return	new ComponentId, or NullId_T if entityId is invalid
			*/
			template <typename T>
			ComponentId addComponentToEntity(T&& component, EntityId entityId) {
				if (!m_entityStore.isValid(entityId)) {
					return NullId_T;
				}
				auto& entity = m_entityStore[entityId];

				auto& store = getComponentStore<T>();
				auto componentId = store.addComponent(std::forward<T>(component), entityId);

				auto previousMask = entity.componentMask;
				entity.addComponent(componentId);
				auto newMask = entity.componentMask;

				if (newMask != previousMask) {
					// fix up the mask index with the new mask, remove the old entry with old mask
					auto rng = m_componentIndex.equal_range(previousMask.to_ullong());
					std::remove_if(rng.first, rng.second, [entityId](ComponentMaskMap::value_type& val){
						return (val.second == entityId);
					});
					
					// insert the new entry with new mask
					m_componentIndex.insert(ComponentMaskMap::value_type{ newMask.to_ullong(), entityId });

					// we can convert the bitset<64> to a uint64_t and sort that, but if more
					// components are needed later will need a bigger mask key as well
					static_assert(sizeof(ComponentMask) <= sizeof(uint64_t),
								  "ComponentMask contains more than 64 bits, need a new sort key for the ComponentMask");
				}

				return componentId;
			}
			
			/**
			* Removes a component from the entity's components vector, and unsets the bit in the
			* ComponentMask if the removed component is the last of its type. The entity id is
			* known from the component record. The EntityManager is not notified by this function,
			* so this should only be used internally or if you REALLY know what you're doing.
			* @param componentId	the component to be removed from its entity
			* @return	true if the component was removed, false if it does not exist
			*/
			bool removeComponentFromEntity(ComponentId componentId);
			
			/**
			* Removes all components of a type from from the entity's components vector, and unsets
			* the corresponding bit in the ComponentMask.
			* @param ct		the component type's enum value
			* @param entityId	the entity to remove from
			* @return	true if the component was removed, false if the entity doesn't exist or
			*	nothing was removed
			*/
			bool removeComponentsOfTypeFromEntity(ComponentType ct, EntityId entityId);


			// Component Store Functions

			/**
			* Get a reference to component store for a specific type, assumed to be a component
			* with ::componentType member. Useful for systems.
			* @tparam T	the component type, requires member T::componentType
			* @return	the ComponentStore for a given type
			*/
			template <typename T>
			ComponentStore<T>& getComponentStore() {
				if (!m_componentStores[T::componentType]) {
					createComponentStore<T>(RESERVE_ENTITYMANAGER_COMPONENTS);
				}
				return *reinterpret_cast<ComponentStore<T>*>(m_componentStores[T::componentType].get());
			}

			/**
			* Create a store for a specific type, assumed to be a component with ::componentType member
			* which identifies the type id and becomes the index into the stores vector.
			* @tparam T	the component type, requires member T::componentType
			*/
			template <typename T>
			void createComponentStore(size_t reserve) {
				assert(T::componentType < ComponentType::last_ComponentType_enum && "static component with dynamic id");

				createComponentStore<T>(T::componentType, reserve);
			}

			/**
			* Create a store for script-based components
			* @return	Index id of the component store. Stores cannot be removed once created so
			*	this index is stable for the lifetime of the EntityManager.
			*/
			uint16_t createScriptComponentStore(size_t reserve);

		private:

			// Private Functions
			
			/**
			* Create a store for a specific type, without assuming that the type has a ::componentType
			* member identiying the type id.
			* @tparam T	the component type, requires member T::componentType
			*/
			template <typename T>
			void createComponentStore(uint16_t typeId, size_t reserve) {
				assert(m_componentStores[typeId] == nullptr && "componentStore already exists");

				m_componentStores[typeId] = std::make_unique<ComponentStore<T>>(typeId, (reserve > RESERVE_ENTITYMANAGER_COMPONENTS ? reserve : RESERVE_ENTITYMANAGER_COMPONENTS));
			}


			// Private Variables
			
			EntityMap			m_entityStore;		//<! Entity indexed by handle, stores relationship to components internally

			ComponentMaskMap	m_componentIndex;	//<! index of sorted ComponentMask for O(log n) entity search by multiple component types
			ComponentStoreMap	m_componentStores;	//<! ComponentStore indexed by componentType, stores component and parent entityId
			
		};

		typedef std::shared_ptr<EntityManager>	EntityManagerPtr;
		typedef std::weak_ptr<EntityManager>	EntityManagerWeakPtr;

	}
}

#endif