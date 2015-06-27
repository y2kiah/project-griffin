/**
* @file Entity.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITY_H_
#define GRIFFIN_ENTITY_H_

#include <boost/container/flat_map.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include "components.h"
#include <utility/memory_reserve.h>


namespace griffin {
	namespace entity {

		// Entity should have relationships to other entities through components
		// Entity is a messaging (event) hub, both sending and handling events, allowing for direct
		// dispatch, broadcast, observer, pub/sub, and event bubbling up and down
		struct Entity {
		public:
			/**
			* Adds component id to the set
			* @return	true if the id is added, false if the id is already present
			*/
			bool addComponent(ComponentId id) {
				ComponentType ct = static_cast<ComponentType>(id.typeId);
				componentMask.set(ct);

				if (std::find(components.begin(), components.end(), id) != components.end()) {
					components.push_back(id);
					return true;
				}

				return false;
			}

			/**
			* Removes id from the set
			* @return	true if component was removed, false if not present
			*/
			bool removeComponent(ComponentId id) {
				ComponentType ct = static_cast<ComponentType>(id.typeId);
				bool hasAnotherMatchingComponentType = false;
				bool removed = false;
				
				for (int c = 0; c < components.size(); ++c) {
					// remove by swapping with the last element and then pop_back
					if (components[c] == id) {
						if (c != components.size() - 1) {
							components[c] = components[components.size() - 1];
						}
						components.pop_back();
						removed = true;
					}
					else if (components[c].typeId == ct) {
						hasAnotherMatchingComponentType = true;
					}
				}

				// if no more components of this type, clear it in the mask
				if (removed && !hasAnotherMatchingComponentType) {
					componentMask.set(ct, false);
				}
				
				return removed;
			}

			/**
			* Removes all components of a type
			* @return true if at least one component was removed
			*/
			bool removeComponentsOfType(ComponentType ct) {
				if (hasComponent(ct)) {
					std::remove_if(components.begin(), components.end(), [ct](ComponentId id){
						return (id.typeId == ct);
					});
					componentMask.set(ct, false);

					return true;
				}
				return false;
			}

			/**
			* Uses the component mask to quickly see if a component type exists in this entity
			* @return	true if the component type exists at least once, false if not
			*/
			inline bool hasComponent(ComponentType ct) const {
				return componentMask[ct];
			}
			

			// Variables

			ComponentMask				componentMask;
			std::vector<ComponentId>	components;

			// Functions

			explicit Entity() {
				components.reserve(RESERVE_ENTITY_COMPONENTS);
			}

			#ifdef GRIFFIN_TOOLS_BUILD
			~Entity();
			#endif
		};


		class EntityManager {
		public:
			// Typedefs
			typedef griffin::handle_map<Entity> EntityMap;
			typedef boost::container::flat_multimap<ComponentMask, EntityId> ComponentMaskMap;
			typedef std::array<std::unique_ptr<ComponentStoreBase>, MAX_COMPONENTS> ComponentStoreMap;

			// Functions

			explicit EntityManager() :
				m_entityStore(0, RESERVE_ENTITYMANAGER_ENTITIES),
				m_componentStores{} // zero-init fills with nullptr
			{}


			// Entity Functions

			/**
			* Creates a new empty entity
			* @return	entity id
			*/
			EntityId createEntity();

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
					auto rng = m_componentIndex.equal_range(previousMask);
					std::remove_if(rng.first, rng.second, [](ComponentMaskMap::value_type& val){
						return (val.second == entityId);
					});
					// insert the new entry with new mask
					m_componentIndex.insert({ newMask, entityId });
				}

				return componentId;
			}


			// Component Store Functions

			/**
			* Get a reference to component store for a specific type, assumed to be a component
			* with ::componentType member. Useful for systems.
			*/
			template <typename T>
			ComponentStore<T>& getComponentStore() {
				return *m_componentStores[T::componentType];
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
			uint16_t createScriptComponentStore(size_t reserve) {
				static int currentDynamicSlot = ComponentType::last_ComponentType_enum;
				assert(currentDynamicSlot < MAX_COMPONENTS && "exceeded MAX_COMPONENTS");

				struct LuaComponent {}; // TEMP
				createComponentStore<LuaComponent>(currentDynamicSlot, reserve);

				++currentDynamicSlot;
				return currentDynamicSlot - 1;
			}

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

	}
}

#endif