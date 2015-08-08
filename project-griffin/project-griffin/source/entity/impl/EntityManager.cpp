/**
* @file EntityManager.cpp
* @author Jeff Kiah
*/
#include <SDL_log.h>
#include <entity/components.h>
#include <entity/EntityManager.h>


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
	return true;
}


/**
* Script Components, 8-byte aligned sizes up to MAX_SCRIPT_COMPONENT_SIZE
*/

#define MAX_SCRIPT_COMPONENT_SIZE	128
template <int SizeBytes>
struct ScriptComponent {
	uint8_t data[SizeBytes];
	static const int _data_size = SizeBytes;
};

typedef struct ScriptComponent<8>	ScriptComponent8;
typedef struct ScriptComponent<16>	ScriptComponent16;
typedef struct ScriptComponent<24>	ScriptComponent24;
typedef struct ScriptComponent<32>	ScriptComponent32;
typedef struct ScriptComponent<40>	ScriptComponent40;
typedef struct ScriptComponent<48>	ScriptComponent48;
typedef struct ScriptComponent<56>	ScriptComponent56;
typedef struct ScriptComponent<64>	ScriptComponent64;
typedef struct ScriptComponent<72>	ScriptComponent72;
typedef struct ScriptComponent<80>	ScriptComponent80;
typedef struct ScriptComponent<88>	ScriptComponent88;
typedef struct ScriptComponent<96>	ScriptComponent96;
typedef struct ScriptComponent<104>	ScriptComponent104;
typedef struct ScriptComponent<112>	ScriptComponent112;
typedef struct ScriptComponent<120>	ScriptComponent120;
typedef struct ScriptComponent<128>	ScriptComponent128;

inline uint32_t findStoreMinimumSize(int componentSize) {
	for (int s = 8; s <= MAX_SCRIPT_COMPONENT_SIZE; s += 8) {
		if (componentSize <= s) { return s; }
	}
	return 0;
}


uint16_t EntityManager::createScriptComponentStore(uint16_t typeId, uint32_t componentSize, size_t reserve)
{
	uint16_t storeIndex = ComponentType::last_ComponentType_enum + typeId;
	assert(storeIndex < MAX_COMPONENTS && "exceeded MAX_COMPONENTS");
	assert(componentSize <= MAX_SCRIPT_COMPONENT_SIZE && "exceeded MAX_SCRIPT_COMPONENT_SIZE");
	
	uint32_t storeMinSize = findStoreMinimumSize(componentSize);

	if (m_componentStores[storeIndex] != nullptr || storeMinSize == 0) {
		throw std::logic_error("script component store already exists or component size out of range");
	}

	switch (storeMinSize) {
		case 8:   { createComponentStore<ScriptComponent8>(storeIndex, reserve); break; }
		case 16:  { createComponentStore<ScriptComponent16>(storeIndex, reserve); break; }
		case 24:  { createComponentStore<ScriptComponent24>(storeIndex, reserve); break; }
		case 32:  { createComponentStore<ScriptComponent32>(storeIndex, reserve); break; }
		case 40:  { createComponentStore<ScriptComponent40>(storeIndex, reserve); break; }
		case 48:  { createComponentStore<ScriptComponent48>(storeIndex, reserve); break; }
		case 56:  { createComponentStore<ScriptComponent56>(storeIndex, reserve); break; }
		case 64:  { createComponentStore<ScriptComponent64>(storeIndex, reserve); break; }
		case 72:  { createComponentStore<ScriptComponent72>(storeIndex, reserve); break; }
		case 80:  { createComponentStore<ScriptComponent80>(storeIndex, reserve); break; }
		case 88:  { createComponentStore<ScriptComponent88>(storeIndex, reserve); break; }
		case 96:  { createComponentStore<ScriptComponent96>(storeIndex, reserve); break; }
		case 104: { createComponentStore<ScriptComponent104>(storeIndex, reserve); break; }
		case 112: { createComponentStore<ScriptComponent112>(storeIndex, reserve); break; }
		case 120: { createComponentStore<ScriptComponent120>(storeIndex, reserve); break; }
		case 128: { createComponentStore<ScriptComponent128>(storeIndex, reserve); break; }
	}

	m_scriptComponentStoreSizes[typeId] = storeMinSize;

	return storeIndex;
}


ComponentId EntityManager::addScriptComponentToEntity(uint16_t typeId, EntityId entityId)
{
	uint16_t storeIndex = ComponentType::last_ComponentType_enum + typeId;
	assert(storeIndex < MAX_COMPONENTS && "typeId out of range");
	
	if (!m_entityStore.isValid(entityId)) {
		return NullId_T;
	}
	auto& entity = m_entityStore[entityId];

	auto store = m_componentStores[storeIndex].get();
	if (store == nullptr) {
		return NullId_T;
	}
	auto componentId = store->addComponent(entityId);

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


void* EntityManager::getScriptComponentData(ComponentId componentId)
{
	assert(componentId.typeId < MAX_COMPONENTS && "typeId out of range");
	
	auto cmpSize = m_scriptComponentStoreSizes[componentId.typeId - ComponentType::last_ComponentType_enum];
	assert(cmpSize > 0 && "typeId store not created yet");

	void* dataPtr = nullptr;
	auto pStore = m_componentStores[componentId.typeId].get();
	if (cmpSize > 0) {
		switch (cmpSize) {
			case 8:   { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent8>*>(pStore)->getComponent(componentId).data; break; }
			case 16:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent16>*>(pStore)->getComponent(componentId).data; break; }
			case 24:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent24>*>(pStore)->getComponent(componentId).data; break; }
			case 32:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent32>*>(pStore)->getComponent(componentId).data; break; }
			case 40:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent40>*>(pStore)->getComponent(componentId).data; break; }
			case 48:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent48>*>(pStore)->getComponent(componentId).data; break; }
			case 56:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent56>*>(pStore)->getComponent(componentId).data; break; }
			case 64:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent64>*>(pStore)->getComponent(componentId).data; break; }
			case 72:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent72>*>(pStore)->getComponent(componentId).data; break; }
			case 80:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent80>*>(pStore)->getComponent(componentId).data; break; }
			case 88:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent88>*>(pStore)->getComponent(componentId).data; break; }
			case 96:  { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent96>*>(pStore)->getComponent(componentId).data; break; }
			case 104: { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent104>*>(pStore)->getComponent(componentId).data; break; }
			case 112: { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent112>*>(pStore)->getComponent(componentId).data; break; }
			case 120: { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent120>*>(pStore)->getComponent(componentId).data; break; }
			case 128: { dataPtr = reinterpret_cast<ComponentStore<ScriptComponent128>*>(pStore)->getComponent(componentId).data; break; }
		}
	}
	return dataPtr;
}
