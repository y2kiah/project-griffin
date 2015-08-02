/**
* @file EntityManager.cpp
* @author Jeff Kiah
*/
#include <SDL_log.h>
#include "../EntityManager.h"
#include "../components.h"

using namespace griffin::entity;


// class EntityManager

EntityId EntityManager::createEntity()
{
	return m_entityStore.emplace();
}


bool EntityManager::removeComponentFromEntity(ComponentId componentId)
{
	auto store = m_componentStores[componentId.typeId].get();
	if (store == nullptr) {
		return false;
	}
	auto entityId = store->getEntityId(componentId);

	bool removed = m_entityStore[entityId].removeComponent(componentId);
	if (!removed) {
		return false;
	}

	// TODO remove from the component mask index
	return store->removeComponent(componentId);
}


bool EntityManager::removeComponentsOfTypeFromEntity(ComponentType ct, EntityId entityId)
{
	if (!m_entityStore.isValid(entityId)) {
		return false;
	}

	auto& entity = m_entityStore[entityId];
	auto store = m_componentStores[ct].get();
	if (store == nullptr) {
		return false;
	}

	entity.removeComponentsOfType(ct);
	
	// TODO remove from the component mask index
	// TODO remove from the component store
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
