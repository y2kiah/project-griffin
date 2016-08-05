/**
* @file EntityManager.cpp
* @author Jeff Kiah
*/
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
* Data Components, 8-byte aligned sizes up to MAX_DATA_COMPONENT_SIZE
*/

#define MAX_DATA_COMPONENT_SIZE	128
template <int SizeBytes>
struct DataComponent {
	uint8_t data[SizeBytes];
	static const int _data_size = SizeBytes;
};

typedef struct DataComponent<8>		DataComponent8;
typedef struct DataComponent<16>	DataComponent16;
typedef struct DataComponent<24>	DataComponent24;
typedef struct DataComponent<32>	DataComponent32;
typedef struct DataComponent<40>	DataComponent40;
typedef struct DataComponent<48>	DataComponent48;
typedef struct DataComponent<56>	DataComponent56;
typedef struct DataComponent<64>	DataComponent64;
typedef struct DataComponent<72>	DataComponent72;
typedef struct DataComponent<80>	DataComponent80;
typedef struct DataComponent<88>	DataComponent88;
typedef struct DataComponent<96>	DataComponent96;
typedef struct DataComponent<104>	DataComponent104;
typedef struct DataComponent<112>	DataComponent112;
typedef struct DataComponent<120>	DataComponent120;
typedef struct DataComponent<128>	DataComponent128;

inline uint32_t findStoreMinimumSize(int componentSize) {
	for (int s = 8; s <= MAX_DATA_COMPONENT_SIZE; s += 8) {
		if (componentSize <= s) { return s; }
	}
	return 0;
}


uint16_t EntityManager::createDataComponentStore(uint16_t typeId, uint32_t componentSize, size_t reserve)
{
	uint16_t storeIndex = ComponentType::last_ComponentType_enum + typeId;
	assert(storeIndex < MAX_COMPONENTS && "exceeded MAX_COMPONENTS");
	assert(componentSize <= MAX_DATA_COMPONENT_SIZE && "exceeded MAX_DATA_COMPONENT_SIZE");
	
	uint32_t storeMinSize = findStoreMinimumSize(componentSize);

	if (m_componentStores[storeIndex] != nullptr || storeMinSize == 0) {
		throw std::logic_error("data component store already exists or component size out of range");
	}

	switch (storeMinSize) {
		case 8:   { createComponentStore<DataComponent8>(storeIndex, reserve); break; }
		case 16:  { createComponentStore<DataComponent16>(storeIndex, reserve); break; }
		case 24:  { createComponentStore<DataComponent24>(storeIndex, reserve); break; }
		case 32:  { createComponentStore<DataComponent32>(storeIndex, reserve); break; }
		case 40:  { createComponentStore<DataComponent40>(storeIndex, reserve); break; }
		case 48:  { createComponentStore<DataComponent48>(storeIndex, reserve); break; }
		case 56:  { createComponentStore<DataComponent56>(storeIndex, reserve); break; }
		case 64:  { createComponentStore<DataComponent64>(storeIndex, reserve); break; }
		case 72:  { createComponentStore<DataComponent72>(storeIndex, reserve); break; }
		case 80:  { createComponentStore<DataComponent80>(storeIndex, reserve); break; }
		case 88:  { createComponentStore<DataComponent88>(storeIndex, reserve); break; }
		case 96:  { createComponentStore<DataComponent96>(storeIndex, reserve); break; }
		case 104: { createComponentStore<DataComponent104>(storeIndex, reserve); break; }
		case 112: { createComponentStore<DataComponent112>(storeIndex, reserve); break; }
		case 120: { createComponentStore<DataComponent120>(storeIndex, reserve); break; }
		case 128: { createComponentStore<DataComponent128>(storeIndex, reserve); break; }
	}

	m_dataComponentStoreSizes[typeId] = storeMinSize;

	return storeIndex;
}


ComponentId EntityManager::addDataComponentToEntity(uint16_t typeId, EntityId entityId)
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


void* EntityManager::getDataComponent(ComponentId componentId)
{
	assert(componentId.typeId < MAX_COMPONENTS && "typeId out of range");
	
	auto cmpSize = m_dataComponentStoreSizes[componentId.typeId - ComponentType::last_ComponentType_enum];
	assert(cmpSize > 0 && "typeId store not created yet");

	void* dataPtr = nullptr;
	auto pStore = m_componentStores[componentId.typeId].get();
	if (cmpSize > 0) {
		switch (cmpSize) {
			case 8:   { dataPtr = reinterpret_cast<ComponentStore<DataComponent8>*>(pStore)->getComponent(componentId).data; break; }
			case 16:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent16>*>(pStore)->getComponent(componentId).data; break; }
			case 24:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent24>*>(pStore)->getComponent(componentId).data; break; }
			case 32:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent32>*>(pStore)->getComponent(componentId).data; break; }
			case 40:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent40>*>(pStore)->getComponent(componentId).data; break; }
			case 48:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent48>*>(pStore)->getComponent(componentId).data; break; }
			case 56:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent56>*>(pStore)->getComponent(componentId).data; break; }
			case 64:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent64>*>(pStore)->getComponent(componentId).data; break; }
			case 72:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent72>*>(pStore)->getComponent(componentId).data; break; }
			case 80:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent80>*>(pStore)->getComponent(componentId).data; break; }
			case 88:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent88>*>(pStore)->getComponent(componentId).data; break; }
			case 96:  { dataPtr = reinterpret_cast<ComponentStore<DataComponent96>*>(pStore)->getComponent(componentId).data; break; }
			case 104: { dataPtr = reinterpret_cast<ComponentStore<DataComponent104>*>(pStore)->getComponent(componentId).data; break; }
			case 112: { dataPtr = reinterpret_cast<ComponentStore<DataComponent112>*>(pStore)->getComponent(componentId).data; break; }
			case 120: { dataPtr = reinterpret_cast<ComponentStore<DataComponent120>*>(pStore)->getComponent(componentId).data; break; }
			case 128: { dataPtr = reinterpret_cast<ComponentStore<DataComponent128>*>(pStore)->getComponent(componentId).data; break; }
		}
	}
	return dataPtr;
}
