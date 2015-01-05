#include "../ResourceCache.h"
#include "../Resource.h"

namespace griffin {
	namespace resource {
		
		template <typename T>
		T& ResourceCache::getResource(Id_T handle)
		{
			// move to back of LRU
			return m_resourceCache[handle]->getResource<T>();
		}

		Id_T ResourceCache::addResource(ResourcePtr resource)
		{
			auto sizeBytes = resource->sizeBytes();
			ensureRoomForBytes(sizeBytes);

			auto id = m_resourceCache.insert(std::move(resource));
			m_usedSizeBytes += sizeBytes;

			// add to LRU

			return id;
		}
		
		bool ResourceCache::removeResource(Id_T handle, bool force)
		{
			bool removed = false;
			// Ensure that we only remove resources with a use count of 1 (meaning the cache is the
			// only holder of the shared_ptr at this point). This will prevent resource memory from
			// floating off into the nether and losing track of it. The cache will always be the
			// last holder of resource memory, unless force is true, which will ignore this
			// restriction and force the removal from cache.
			if (m_resourceCache.isValid(handle)) {
				const auto& resourcePtr = m_resourceCache[handle];
				if (resourcePtr.use_count() == 1 || force) {
					auto size = resourcePtr->sizeBytes();
					m_usedSizeBytes -= size;
					m_resourceCache.erase(handle);
					removed = true;
				}

			}
			return removed;
		}

		void ResourceCache::ensureRoomForBytes(size_t sizeBytes)
		{
			assert(sizeBytes <= m_maxSizeBytes);

			while (m_maxSizeBytes - m_usedSizeBytes < sizeBytes) {
				// get front from LRU
				//removeResource(handle);
			}
		}

	}
}