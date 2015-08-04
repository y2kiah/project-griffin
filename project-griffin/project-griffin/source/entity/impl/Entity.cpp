/**
* @file Entity.cpp
* @author Jeff Kiah
*/
#include "../Entity.h"
#include <SDL_log.h>

using namespace griffin::entity;


// class Entity

bool Entity::addComponent(ComponentId id)
{
	ComponentType ct = static_cast<ComponentType>(id.typeId);
	componentMask.set(ct);

	if (std::find(components.begin(), components.end(), id) == components.end()) {
		components.push_back(id);
		return true;
	}

	return false;
}


bool Entity::removeComponent(ComponentId id)
{
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


bool Entity::removeComponentsOfType(ComponentType ct)
{
	if (hasComponent(ct)) {
		std::remove_if(components.begin(), components.end(), [ct](ComponentId id){
			return (id.typeId == ct);
		});
		componentMask.set(ct, false);

		return true;
	}
	return false;
}


#ifdef GRIFFIN_TOOLS_BUILD
Entity::~Entity()
{
	if (components.capacity() > RESERVE_ENTITY_COMPONENTS) {
		SDL_Log("check RESERVE_ENTITY_COMPONENTS: original=%d, highest=%d", RESERVE_ENTITY_COMPONENTS, components.capacity());
	}
}
#endif
