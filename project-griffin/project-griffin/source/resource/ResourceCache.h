#pragma once

#include <string>
#include <memory>
#include <utility/container/handle_map.h>

namespace griffin {
	namespace resource {

		using std::wstring;

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

			Id_T addResource(ResourcePtr resource, wstring name);

			void setBytesFreed(size_t sizeBytes) {
				m_usedSizeBytes -= sizeBytes;
			}

			void makeRoomForBytes(size_t sizeBytes)
			{
				
			}

		private:
			size_t			m_maxSizeBytes;
			size_t			m_usedSizeBytes;

			ResourceMap		m_resourceCache;
		};
	}
}