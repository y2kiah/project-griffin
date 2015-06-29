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
#include "Entity.h"
#include "components.h"


namespace griffin {
	namespace entity {

		class EntityManager {
		public:
			// Typedefs
			typedef griffin::handle_map<Entity> EntityMap;
			typedef boost::container::flat_multimap<uint64_t, EntityId> ComponentMaskMap;
			typedef std::array<std::unique_ptr<ComponentStoreBase>, MAX_COMPONENTS> ComponentStoreMap;

			// Functions

			explicit EntityManager() :
				m_entityStore(0, RESERVE_ENTITYMANAGER_ENTITIES),
				m_componentStores{} // zero-init fills with nullptr
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
			* Get the ComponentIds that belong to an entity
			*/
			const std::vector<ComponentId>& getEntityComponents(EntityId entityId) const {
				return m_entityStore[entityId].components;
			}

			/**
			* Adds a component to an existing entity
			* @param entityId	entity to receive the new component
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
			*
			*/
			bool removeComponentFromEntity(ComponentId componentId);
			
			/**
			*
			*/
			bool removeComponentsOfTypeFromEntity(ComponentType ct, EntityId entityId);


			// Component Store Functions

			/**
			* Get a reference to component store for a specific type, assumed to be a component
			* with ::componentType member. Useful for systems.
			*/
			template <typename T>
			ComponentStore<T>& getComponentStore() {
				return *reinterpret_cast<ComponentStore<T>*>(m_componentStores[T::componentType].get());
			}

			/**
			* Create a store for a specific type, assumed to be a component with ::componentType member
			* which identifies the type id and becomes the index into the stores vector.
			* @return	true if store was created, false if it already existed
			*/
			template <typename T>
			bool createComponentStore(size_t reserve) {
				assert(T::componentType < ComponentType::last_ComponentType_enum && "static component with dynamic id");

				return createComponentStore<T>(T::componentType, reserve);
			}

			/**
			* Create a store for script-based components
			*/
			uint16_t createScriptComponentStore(size_t reserve);

		private:

			// Private Functions
			
			/**
			* Create a store for a specific type, without assuming that the type has a ::componentType
			* member identiying the type id.
			*/
			template <typename T>
			void createComponentStore(uint16_t typeId, size_t reserve) {
				assert(m_componentStores[currentDynamicSlot] == nullptr && "componentStore already exists");

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