/**
* @file   Entity.cpp
* @author Jeff Kiah
*/
#include <mutex>
#include <memory>
#include <entity/Entity.h>
#include <SDL_log.h>

using namespace griffin::entity;


// class Entity

#ifdef GRIFFIN_TOOLS_BUILD
Entity::~Entity()
{
	if (components.capacity() > RESERVE_ENTITY_COMPONENTS) {
		SDL_Log("check RESERVE_ENTITY_COMPONENTS: original=%d, highest=%d", RESERVE_ENTITY_COMPONENTS, components.capacity());
	}
}
#endif


// class EntityManager

EntityId EntityManager::createEntity()
{
	return m_entityStore.emplace();
}
