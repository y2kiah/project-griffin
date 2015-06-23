#pragma once
#ifndef GRIFFIN_COMPONENTSTORE_H_
#define GRIFFIN_COMPONENTSTORE_H_

#include <utility/container/handle_map.h>
#include <string>

namespace griffin {
	namespace entity {

		typedef griffin::Id_T    ComponentId;
		typedef griffin::IdSet_T ComponentIdList;
		typedef griffin::Id_T    EntityId;

		inline std::ostream& operator<<(std::ostream& os, ComponentId id);

		/**
		* @class ComponentStoreBase
		* Exists only to allow storage of base class pointers in homogenous array
		*/
		class ComponentStoreBase {};

		/**
		*
		*/
		template <typename T>
		class ComponentStore : public ComponentStoreBase {
		public:
			// Typedefs
			typedef struct {
				T        component;
				EntityId entityId;
			} ComponentRecord;

			typedef griffin::handle_map<ComponentRecord> ComponentMap;

			// Functions

			/**
			* get a const ref to the components handle_map, useful for systems
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
			inline ComponentIdList createComponents(int n, EntityId entityId) {
				return m_components.emplaceItems(n, T{}, entityId);
			}

			/**
			* add one component, moving the provided cmp into the store, return ComponentId
			*/
			inline ComponentId addComponent(T&& cmp, EntityId entityId) {
				return m_components.insert({ std::forward<T>(cmp), entityId });
			}

			/**
			* remove the component identified by the provided outerId
			*/
			inline void removeComponent(ComponentId outerId) {
				m_components.removeItem(outerId);
			}

			/**
			* Get struct containing reference to Component, and parent entityId. See notes on getComponent
			* for storage of references to the component.
			*/
			inline const ComponentRecord& getComponentRecord(ComponentId outerId) {
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