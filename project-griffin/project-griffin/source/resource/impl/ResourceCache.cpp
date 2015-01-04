#include "../ResourceCache.h"
#include "../Resource.h"

namespace griffin {
	namespace resource {
	
		Id_T ResourceCache::addResource(ResourcePtr resource, wstring name)
		{
			auto sizeBytes = resource->sizeBytes();
			auto difference = m_usedSizeBytes + sizeBytes - m_maxSizeBytes;
			if (difference > 0) {
				makeRoomForBytes(difference);
			}

			auto id = m_resourceCache.insert(std::move(resource));
			m_usedSizeBytes += sizeBytes;
			return id;
		}
	
	}
}