/*
desired interface:
	entity.addComponent(Position_T);
	entity.removeComponent(Orientation_T);
*/

#pragma once
#ifndef _ENTITY_H
#define _ENTITY_H

#include <map>
#include <memory>
#include "Component.h"

typedef std::map<ComponentType, size_t>	ComponentStoreIndex;


class Entity {
public:
	bool addComponent(ComponentType ct) {
		if (!hasComponent(ct)) {
			componentMask.set(ct);
			components[ct] = 0; // get id from componentstore
			return true;
		}
		return false;
	}

	bool removeComponent(ComponentType ct) {
		if (hasComponent(ct)) {
			componentMask.set(ct, false);
			// remove from componentstore
			components.erase(ct);
		}
		return false;
	}

	inline bool hasComponent(ComponentType ct) const {
		return componentMask[ct];
	}

	Entity::~Entity() {

	}

private:
	ComponentMask		componentMask;
	ComponentStoreIndex	components;

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