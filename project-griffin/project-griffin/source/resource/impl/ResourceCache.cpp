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
			auto difference = m_usedSizeBytes + sizeBytes - m_maxSizeBytes;
			if (difference > 0) {
				makeRoomForBytes(difference);
			}

			auto id = m_resourceCache.insert(std::move(resource));
			m_usedSizeBytes += sizeBytes;

			// add to LRU

			return id;
		}
		
		bool ResourceCache::removeResource(Id_T handle, bool force)
		{
			// Ensure that we only remove resources with a use count of 1 (meaning the cache is the
			// only holder of the shared_ptr at this point). This will prevent resource memory from
			// floating off into the nether and losing track of it. The cache will always be the
			// last holder of resource memory, unless force is true, which will ignore this
			// restriction and force the removal from cache.
			if (!m_resourceCache.isValid(handle) ||
				(m_resourceCache[handle].use_count > 1 && !force))
			{
				return false;
			}

			size_t size = m_resourceCache[handle]->sizeBytes();
			m_resourceCache.erase(handle);
			m_usedSizeBytes -= size;

			return true;
		}

		void ResourceCache::makeRoomForBytes(size_t sizeBytes)
		{
		}

	}
}