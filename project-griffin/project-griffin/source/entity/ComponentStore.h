#pragma once
#ifndef GRIFFIN_COMPONENTSTORE_H_
#define GRIFFIN_COMPONENTSTORE_H_

#include <utility/container/handle_map.h>
#include <string>
#include "EntityTypedefs.h"


namespace griffin {
	namespace entity {

		inline std::ostream& operator<<(std::ostream& os, ComponentId id);

		/**
		* @class ComponentStoreBase
		* Exists only to allow storage of base class pointers in homogenous array
		*/
		class ComponentStoreBase {
		public:
			virtual ~ComponentStoreBase() {}

			virtual ComponentId addComponent(EntityId entityId) = 0;
			virtual bool removeComponent(ComponentId outerId) = 0;
			virtual EntityId getEntityId(ComponentId outerId) = 0;
		};

		/**
		* @class ComponentStore
		* Wrapper around a handle_map specifically for storing the components of the ECS.
		* Each item in the internal data contains the component object and its parent EntityId.
		*/
		template <typename T>
		class ComponentStore : public ComponentStoreBase {
		public:
			// Typedefs

			/**
			* ComponentRecord stores the component object plus its parent entity id. This provides
			* a fast index to find all entities that contain this component type.
			*/
			typedef struct {
				T        component;
				EntityId entityId;
			} ComponentRecord;

			typedef griffin::handle_map<ComponentRecord> ComponentMap;

			// Functions

			/**
			* get the components handle_map, useful for systems
			*/
			inline ComponentMap& getComponents() {
				return m_components;
			}

			/**
			* get the components handle_map, useful for systems
			*/
			inline const ComponentMap& getComponents() const {
				return m_components;
			}

			/**
			* create one component with default zero-initialization
			*/
			inline ComponentId createComponent(EntityId entityId) {
				return m_components.insert({ T{}, entityId });
			}

			/**
			* create n components and return a vector of their ComponentIds
			*/
			inline ComponentIdSet createComponents(int n, EntityId entityId) {
				return m_components.emplaceItems(n, T{}, entityId);
			}

			/**
			* add one component, moving the provided cmp into the store, return ComponentId
			*/
			inline ComponentId addComponent(T&& cmp, EntityId entityId) {
				return m_components.insert({ std::forward<T>(cmp), entityId });
			}

			/**
			* add one zero-initialized component into the store, return ComponentId
			*/
			virtual inline ComponentId addComponent(EntityId entityId) override {
				return m_components.insert({ T{}, entityId });
			}

			/**
			* remove the component identified by the provided outerId
			*/
			virtual inline bool removeComponent(ComponentId outerId) override {
				return (m_components.erase(outerId) == 1);
			}

			/**
			* Get struct containing reference to Component, and parent entityId. See notes on getComponent
			* for storage of references to the component.
			*/
			inline const ComponentRecord& getComponentRecord(ComponentId outerId) {
				return m_components[outerId];
			}

			/**
			* Get the component and parent EntityId for a component
			*/
			inline ComponentRecord& operator[](ComponentId outerId) {
				return m_components[outerId];
			}

			/**
			* Get the component and parent EntityId for a component
			*/
			inline const ComponentRecord& operator[](ComponentId outerId) const {
				return m_components[outerId];
			}

			/**
			* Don't store the reference for longer than the duration of a single system's frame tick. It
			* would break the model of systems operating on queried entity lists on a per-frame basis. It
			* would also be unsafe to hold a reference to invalid memory, incase the store is deleted.
			*/
			inline T& getComponent(ComponentId outerId) {
				return m_components[outerId].component;
			}

			/**
			* Get the entityId of the parent Entity for a component
			*/
			inline EntityId getEntityId(ComponentId outerId) {
				return m_components[outerId].entityId;
			}

			/**
			* Get the ComponentId for a component iterator, useful inside of range-based for loops.
			*/
			inline ComponentId getComponentIdForItem(typename ComponentMap::DenseSet_T::const_iterator it)
			{
				return m_components.getHandleForItem(it);
			}

			/**
			* to_string for debug and test output
			*/
			std::string to_string() const;

			/**
			* Constructor takes a reserveCount to initialize the inner storage, and the itemTypeId is
			* automatically set to T::componentType
			*/
			explicit ComponentStore(size_t reserveCount) :
				m_components(T::componentType, reserveCount)
			{}

			/**
			* Constructor takes a reserveCount to initialize the inner storage, and the itemTypeId is also
			* passed in. This overload can be used if the typeId is not known at compile time, such as for
			* components that are defined at runtime by schema files or scripting languages. The type T in
			* that case would likely be homogenous
			*/
			explicit ComponentStore(uint16_t typeId, size_t reserveCount) :
				m_components(typeId, reserveCount)
			{}

			ComponentStore(const ComponentStore &) = delete;

		private:
			// Member Variables

			ComponentMap m_components;
		};

	}
}

#include "impl/ComponentStore-inl.h"

#endif