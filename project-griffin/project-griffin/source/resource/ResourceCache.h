#pragma once
#ifndef GRIFFIN_RESOURCE_CACHE_H
#define GRIFFIN_RESOURCE_CACHE_H

#include <memory>
#include <utility/container/handle_map.h>

namespace griffin {
	namespace resource {

		class Resource_T;

		/**
		 *
		 */
		class ResourceCache {
		public:
			typedef std::shared_ptr<Resource_T> ResourcePtr;
			typedef handle_map<ResourcePtr> ResourceMap;

			explicit ResourceCache(uint16_t itemTypeId, size_t reserveCount, size_t maxSizeBytes) :
				m_maxSizeBytes{ maxSizeBytes },
				m_usedSizeBytes{ 0 },
				m_resourceCache(itemTypeId, reserveCount)
			{}

			/**
			 * @returns true if the cache contains the resource identified by handle
			 */
			bool hasResource(Id_T handle) const
			{
				return m_resourceCache.isValid(handle);
			}

			template <typename T>
			T& getResource(Id_T handle);

			Id_T addResource(ResourcePtr resource);

			bool removeResource(Id_T handle, bool force = false);

			void makeRoomForBytes(size_t sizeBytes);

		private:
			size_t			m_maxSizeBytes;
			size_t			m_usedSizeBytes;

			ResourceMap		m_resourceCache;
			//lru_set<Id_T>	m_lru;
		};
	}
}

#endif