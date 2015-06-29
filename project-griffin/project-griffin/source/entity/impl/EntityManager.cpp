/**
* @file EntityManager.cpp
* @author Jeff Kiah
*/
#include <SDL_log.h>
#include "../EntityManager.h"

using namespace griffin::entity;


// class EntityManager

EntityId EntityManager::createEntity()
{
	return m_entityStore.emplace();
}


bool EntityManager::removeComponentFromEntity(ComponentId componentId)
{
	auto store = m_componentStores[componentId.typeId].get();
	auto entityId = store->getEntityId(componentId);

	bool removed = m_entityStore[entityId].removeComponent(componentId);
	if (!removed) { return false; }

	store->removeComponent(componentId);

	return true;
}


uint16_t EntityManager::createScriptComponentStore(size_t reserve)
{
	static int currentDynamicSlot = ComponentType::last_ComponentType_enum;
	assert(currentDynamicSlot < MAX_COMPONENTS && "exceeded MAX_COMPONENTS");

	struct LuaComponent {}; // TEMP
	createComponentStore<LuaComponent>(currentDynamicSlot, reserve);

	++currentDynamicSlot;
	return currentDynamicSlot - 1;
}
