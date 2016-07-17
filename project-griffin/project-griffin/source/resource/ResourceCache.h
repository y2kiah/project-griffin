/**
* @file		ResourceCache.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_CACHE_H_
#define GRIFFIN_RESOURCE_CACHE_H_

#pragma warning(disable:4003)	// not enough actual parameters for macro 'BOOST_PP_EXPAND_I' 

#include <utility/container/handle_map.h>
#include <utility/enum.h>
#include "ResourceTypedefs.h"

namespace griffin {
	namespace resource {

		MakeEnum(CacheType, uint16_t,
				 (Cache_Permanent)
				 (Cache_Materials)
				 (Cache_Models)
				 (Cache_Scripts)
				 ,);

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


			/**
			* @param	maxSizeBytes	pass 0 for infinite size, or max bytes before resources
			*	start falling off the back of the LRU list
			*/
			explicit ResourceCache(uint16_t itemTypeId, size_t reserveCount, size_t maxSizeBytes) :
				m_maxSizeBytes{ maxSizeBytes },
				m_usedSizeBytes{ 0 },
				m_lruBack{},
				m_lruFront{},
				m_resourceCache(itemTypeId, reserveCount),
				m_initialReserve{ reserveCount }
			{}

			~ResourceCache();

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

			size_t		m_initialReserve;
		};


		// Inline Functions

		inline bool ResourceCache::hasResource(Id_T handle) const
		{
			return m_resourceCache.isValid(handle);
		}


		inline ResourcePtr& ResourceCache::getResource(Id_T handle)
		{
			if (m_maxSizeBytes != 0) {
				// if max size is not infinite, maintain the LRU list
				setLRUMostRecent(handle);
			}
			return m_resourceCache[handle].resourcePtr;
		}
	}
}

#endif