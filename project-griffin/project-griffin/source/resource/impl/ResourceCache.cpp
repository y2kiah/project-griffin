#include "../ResourceCache.h"
#include "../Resource.h"

namespace griffin {
	namespace resource {
		
		template <typename T>
		T& ResourceCache::getResource(Id_T handle)
		{
			setLRUMostRecent(handle);
			return m_resourceCache[handle].resourcePtr->getResource<T>();
		}

		Id_T ResourceCache::addResource(ResourcePtr resource)
		{
			auto sizeBytes = resource->sizeBytes();
			ensureRoomForBytes(sizeBytes);

			auto handle = m_resourceCache.insert({
				std::move(resource),
				m_lruBack,
				Id_T{}
			});
			m_usedSizeBytes += sizeBytes;

			setLRUMostRecent(handle);

			return handle;
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
				const auto& thisItem = m_resourceCache[handle];
				if (thisItem.resourcePtr.use_count() == 1 || force) {
					// fix up next/prev for surrounding items
					if (thisItem.previous.value != 0) {
						m_resourceCache[thisItem.previous].next = thisItem.next;
					}
					else { // no previous item, this was the front
						m_lruFront = thisItem.next;
					}
					if (thisItem.next.value != 0) {
						m_resourceCache[thisItem.next].previous = thisItem.previous;
					} else { // no next item, this was the back
						m_lruBack = thisItem.previous;
					}

					// decrement size
					auto size = thisItem.resourcePtr->sizeBytes();
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
				// remove the least recently used resource
				auto handle = m_lruFront;
				removeResource(handle);
			}
		}

		void ResourceCache::setLRUMostRecent(Id_T handle)
		{
			auto& thisItem = m_resourceCache[handle];
			
			// fix up next/prev for surrounding items
			if (thisItem.previous.value != 0) {
				m_resourceCache[thisItem.previous].next = thisItem.next;
			}
			if (thisItem.next.value != 0) {
				m_resourceCache[thisItem.next].previous = thisItem.previous;
			}

			// fix list back
			if (m_lruBack.value != 0) {
				m_resourceCache[m_lruBack].next = handle;
			}
			m_lruBack = handle;
			
			// fix list front
			if (m_lruFront.value == 0) {
				m_lruFront = handle;
			}
		}
	}
}