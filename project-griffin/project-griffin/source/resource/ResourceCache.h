/**
* @file		ResourceCache.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_CACHE_H_
#define GRIFFIN_RESOURCE_CACHE_H_

#include <memory>
#include <utility/container/handle_map.h>
#include <utility/enum.h>

namespace griffin {
	namespace resource {

		using std::shared_ptr;
		using std::unique_ptr;

		class Resource_T;
		typedef shared_ptr<Resource_T> ResourcePtr;

		MakeEnum(CacheType, uint16_t,
				 (Cache_Materials)
				 (Cache_Models)
				 (Cache_Scripts)
				 , _T);

		/**
		*
		*/
		class ResourceCache {
		public:
			struct ResourceLRUItem {
				ResourcePtr		resourcePtr;
				Id_T			previous;
				Id_T			next;
			};
			typedef handle_map<ResourceLRUItem> ResourceMap;


			explicit ResourceCache(uint16_t itemTypeId, size_t reserveCount, size_t maxSizeBytes) :
				m_maxSizeBytes{ maxSizeBytes },
				m_usedSizeBytes{ 0 },
				m_lruBack{},
				m_lruFront{},
				m_resourceCache(itemTypeId, reserveCount)
			{}

			/**
			* @returns true if the cache contains the resource identified by handle
			*/
			bool hasResource(Id_T handle) const;

			ResourcePtr& getResource(Id_T handle);

			Id_T addResource(ResourcePtr resource);

			bool removeResource(Id_T handle, bool force = false);

			void ensureRoomForBytes(size_t sizeBytes);

			void setLRUMostRecent(Id_T handle);

			uint16_t getItemTypeId() const { return m_resourceCache.getItemTypeId(); }

		private:
			size_t		m_maxSizeBytes;
			size_t		m_usedSizeBytes;
			Id_T		m_lruFront;	// least recent
			Id_T		m_lruBack;	// most recent

			ResourceMap	m_resourceCache;
		};

		typedef shared_ptr<ResourceCache>	ResourceCachePtr;


		// Inline Functions

		inline bool ResourceCache::hasResource(Id_T handle) const
		{
			return m_resourceCache.isValid(handle);
		}


		inline ResourcePtr& ResourceCache::getResource(Id_T handle)
		{
			setLRUMostRecent(handle);
			return m_resourceCache[handle].resourcePtr;
		}
	}
}

#endif