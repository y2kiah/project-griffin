/**
* @file		ResourceLoader.cpp
* @author	Jeff Kiah
*/
#include "../ResourceCache.h"
#include "../Resource.h"
#include <utility/Logger.h>

namespace griffin {
	namespace resource {

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

			if (m_maxSizeBytes != 0) {
				// if max size is not infinite, maintain the LRU list
				setLRUMostRecent(handle);
			}

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
			assert(sizeBytes <= m_maxSizeBytes || m_maxSizeBytes == 0);

			if (m_maxSizeBytes != 0) { // zero max size means infinite size
				while (m_maxSizeBytes - m_usedSizeBytes < sizeBytes) {
					// remove the least recently used resource
					auto handle = m_lruFront;
					removeResource(handle);
				}
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


		ResourceCache::~ResourceCache()
		{
			if (m_resourceCache.capacity() > m_initialReserve) {
				logger.info("check ResourceCache %d RESERVE: original=%d, highest=%d", m_resourceCache.getItemTypeId(), m_initialReserve, m_resourceCache.capacity());
			}
		}

	}
}