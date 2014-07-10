/*
desired interface:
	entity.addComponent(Position_T);
	entity.removeComponent(Orientation_T);
*/

#pragma once
#ifndef ENTITY_H
#define ENTITY_H

#include <boost/container/flat_set.hpp>
//#include <boost/container/flat_map.hpp>
#include <memory>
#include "components.h"

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
	*
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

	bool removeComponentsOfType(ComponentType ct) {
		if (hasComponent(ct)) {
			componentMask.set(ct, false);
			
			auto lower = components.lower_bound({{{0, 0, ct, 0}}});
			auto upper = components.upper_bound({{{0xFFFFFFFF, 0xFFFF, ct, 0}}});
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

	// Entity should have an optional parent entity and optional child entities

	// Entity is a messaging (event) hub, both sending and handling events, allowing for direct
	// dispatch, broadcast, observer, pub/sub, and event bubbling up and down
};


class EntityFactory {
public:
	std::shared_ptr<Entity> createEntity(ComponentMask componentMask);

private:


};

#endif

/**
 *	Entity does not contain component data directly, it is a structure used to aggregate indexes
 *	into the various component stores that hold component data. Entity will also provide a
 *	convenient place to implement game object reflection, serialization, integration with scripting
 *	engine, and possibly generating network packets.
 *
 *	Copyright © 2013 - Jeff Kiah - All rights reserved.
 */