/**
* @file   Entity.h
* @author Jeff Kiah
*/

#pragma once
#ifndef GRIFFIN_ENTITY_H_
#define GRIFFIN_ENTITY_H_

#include <boost/container/flat_set.hpp>
//#include <boost/container/flat_map.hpp>
#include <memory>
#include "components.h"
#include <utility/memory_reserve.h>


namespace griffin {
	namespace entity {

		typedef boost::container::flat_set<ComponentId> ComponentSet;
		//typedef boost::container::flat_multimap<ComponentType, ComponentId> ComponentMap;


		class Entity {
		public:
			/**
			* Adds id to the set, returns true if the id is added, false if the id is already present
			*/
			bool addComponent(ComponentId id) {
				ComponentType ct = static_cast<ComponentType>(id.typeId);
				componentMask.set(ct);

				auto r = components.emplace(id);
				//components.emplace(ct, id);

				return r.second;
			}

			/**
			* Removes id from the set, returns true if component was removed, false if not present
			*/
			bool removeComponent(ComponentId id) {
				ComponentType ct = static_cast<ComponentType>(id.typeId);
				auto r = components.erase(id);

				// implementation for multimap
				/*if (hasComponent(ct)) {
					auto keyRange = components.equal_range(ct);
					int keysMatched = std::distance(keyRange.first, keyRange.second);

					// need to test, does this delete only keys with matching id value?
					std::remove_if(keyRange.first, keyRange.second, [id, &keysMatched](const std::pair<ComponentType, ComponentId>& it){
					bool match = (it.second == id);
					if (match) { --keysMatched; } // decrement number matched if removed
					return match;
					});

					// if all matched keys were removed, unset component type in the mask
					if (keysMatched == 0) {
					componentMask.set(ct, false);
					}

					return true;
					}*/
				return (r > 0);
			}

			/**
			* Removes all components of a type, returns true if at least one component was removed
			*/
			bool removeComponentsOfType(ComponentType ct) {
				if (hasComponent(ct)) {
					componentMask.set(ct, false);

					auto lower = components.lower_bound({ { { 0, 0, ct, 0 } } });
					auto upper = components.upper_bound({ { { 0xFFFFFFFF, 0xFFFF, ct, 0 } } });
					components.erase(lower, upper);
					//auto it = components.
					//components.erase(ct);

					// remove from componentstore
					return true;
				}
				return false;
			}

			inline bool hasComponent(ComponentType ct) const {
				return componentMask[ct];
			}

			Entity::~Entity() {
				components.reserve(RESERVE_ENTITY_COMPONENTS);
			}

		private:
			ComponentMask	componentMask;
			ComponentSet	components;
			//ComponentMap	components;

			// component mask could also be stored in array in the entitystore for indexed searches,
			// OR should entitystore keep an array of indexes for each component type, which would be
			// an O(1) lookup vs O(N) with the mask, but at the cost of slightly more expensive
			// addComponent and removeComponent functions, since they now have to also "register" or
			// "unregister" in these master index arrays

			// Entity should have relationships to other entities through components

			// Entity is a messaging (event) hub, both sending and handling events, allowing for direct
			// dispatch, broadcast, observer, pub/sub, and event bubbling up and down
		};


		class EntityManager {
		public:
			// Typedefs
			typedef griffin::handle_map<Entity> EntityMap;
			typedef std::vector<std::unique_ptr<ComponentStoreBase>> ComponentStoreMap;

			// Functions

			explicit EntityManager() :
				entityStore(0, RESERVE_ENTITYMANAGER_ENTITIES)
			{
				componentStores.reserve(RESERVE_ENTITYMANAGER_COMPONENTSTORES);
			}

			/**
			* Create a store for a specific type, without assuming that the type has a ::componentType
			* member identiying the type id.
			*/
			template <typename T>
			bool createComponentStore(uint16_t typeId, size_t reserve) {
				// ensure vector is resized to fit up to componentType elements
				if (componentStores.size() < typeId) {
					componentStores.reserve(typeId);
					componentStores.resize(typeId, nullptr);
				}
				// create store if it doesn't exist already
				auto &store = componentStores[typeId];
				if (!store()) {
					store = std::make_unique<ComponentStore<T>>(typeId, reserve);
					return true;
				}
				return false;
			}

			/**
			* Create a store for a specific type, assumed to be a component with ::componentType member
			* which identifies the type id and becomes the index into the stores vector.
			* @returns true if store was created, false if it already existed
			*/
			template <typename T>
			bool createComponentStore(size_t reserve) {
				return createComponentStore(T::componentType, reserve);
			}


			std::shared_ptr<Entity> createEntity(ComponentMask componentMask); // TEMP

		private:
			// Entity indexed by handle, stores relationship to components internally
			EntityMap         entityStore;

			// ComponentStore indexed by componentType, stores component and parent entityId
			ComponentStoreMap componentStores;


		};

		typedef std::shared_ptr<EntityManager>	EntityManagerPtr;

	}
}

#endif