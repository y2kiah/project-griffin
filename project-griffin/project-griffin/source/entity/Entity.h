/**
* @file Entity.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_ENTITY_H_
#define GRIFFIN_ENTITY_H_

#include <vector>
#include <utility/memory_reserve.h>
#include "components.h"


namespace griffin {
	namespace entity {

		// Entity should have relationships to other entities through components
		// Entity is a messaging (event) hub, both sending and handling events, allowing for direct
		// dispatch, broadcast, observer, pub/sub, and event bubbling up and down
		struct Entity {
			explicit Entity() {
				components.reserve(RESERVE_ENTITY_COMPONENTS);
			}

			#ifdef GRIFFIN_TOOLS_BUILD
			~Entity();
			#endif

			/**
			* Adds component id to the set
			* @return	true if the id is added, false if the id is already present
			*/
			bool addComponent(ComponentId id);

			/**
			* Removes id from the set
			* @return	true if component was removed, false if not present
			*/
			bool removeComponent(ComponentId id);

			/**
			* Removes all components of a type
			* @return true if at least one component was removed
			*/
			bool removeComponentsOfType(ComponentType ct);

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
		};

	}
}

#endif