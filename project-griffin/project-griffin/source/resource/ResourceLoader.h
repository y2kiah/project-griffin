/**
* @file		ResourceLoader.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_LOADER_
#define GRIFFIN_RESOURCE_LOADER_

#include <string>
#include <memory>
#include <future>
//#include <functional>
//#include <boost/container/flat_map.hpp>
#include <unordered_map>
//#include <sparsehash/dense_hash_map>
#include <array>
#include <utility/concurrency.h>
#include <core/CoreSystem.h>
#include "Resource.h"
#include "ResourceCache.h"
#include "ResourceSource.h"

namespace griffin {
	namespace resource {

		using std::unique_ptr;
		using std::shared_ptr;
		using std::wstring;
		using griffin::core::CoreSystem;

		/**
		*
		*/
		class ResourceLoader : public CoreSystem {
		public:
			/**
			* Signature: resource shared_ptr, resource handle, resource size
			*/
			typedef std::function<void(const ResourcePtr&, Id_T, size_t)>	CallbackFunc_T;

			explicit ResourceLoader() :
				m_c{}, m_callbacks{}
			{}

			/**
			* update on the update thread calls queued callbacks
			*/
			virtual void update(const UpdateInfo& ui) override;
			
			/**
			* executing on each frame currently, not tied to the update fixed timestep
			*/
			void executeCallbacks();

			/**
			*
			*/
			template <typename T, typename BuilderFunc>
			ResourceHandle<T> load(const wstring &name, CacheType cache,
								   BuilderFunc&& builder, CallbackFunc_T callback = nullptr);

			/**
			*
			*/
			template <typename T>
			std::future<ResourcePtr> getResource(const ResourceHandle<T>& h);

			void registerCache(const ResourceCachePtr& cachePtr, CacheType cacheTypeId);
			void registerSource(const IResourceSourcePtr& sourcePtr);

		private:
			typedef std::array<ResourceCachePtr, CacheTypeCount>	ResourceCacheSet;
			typedef std::vector<IResourceSourcePtr>					ResourceSourceSet;
			//typedef boost::container::flat_map<std::wstring, Id_T>	ResourceNameIndex;
			typedef std::unordered_map<wstring, Id_T>				ResourceNameIndex;
			//typedef dense_hash_map<wstring, Id_T>					ResourceNameIndex;
			
			struct Impl {
				ResourceCacheSet	m_caches;
				ResourceSourceSet	m_sources;
				ResourceNameIndex	m_nameToHandle;
			};

			// Variables

			concurrent<Impl> m_c;
			
			concurrent_queue<std::function<void()>> m_callbacks;
		};

		typedef shared_ptr<ResourceLoader>	ResourceLoaderPtr;

	}
}

#include "impl/ResourceLoader-inl.h"

/*
Overview:
	The resource system is responsible for streaming binary data from disk and constructing
	resource objects from that data. The system should allow for resource requests by name (e.g.
	file system path/name), and by handle. Lookups by name are expected to be slower and done
	sparingly, while lookups by handle are faster and done more often throughout. Handles, not
	names, are expected to be copied and stored in order to refer back to the unique resource.

Goals:
	1. asynchronous in nature, non-blocking API
	2. handle multiple simultaneous requests for the same resource without duplication
	4. allow resources of various types to be loaded maintaining type safety
	5. allow clients to implement a caching algorithm independant of the internal LRU cache
	6. allow a single container to store many different resource types
	7. don't be intrusive, resource implementers don't inherit from a common base class, prefer
		duck typing over polymorphism

Myths (to dispell):
	1. using shared_ptr is a performance concern, causes synchronization - Copies are rare,
		synchronization only occurs when banging on the same shared_ptr at the same time, which is
		unlikely with this system.

Gotchas (things to watch out for):
	1. storing shared_ptrs to resources within other resources will possibly circumvent the caching
		system, making the cache algorithm (e.g. LRU) of dependencies moot since the cache of the
		parent resource will now dictate when the dependency gets released. Parent resources are
		likely higher-level and fewer in count, and are less likely to be released by a LRU
		mechanism, thus the LRU scheme for lower level resources stops working as intended. Prefer
		storing handles to the dependency only, which will not force the child resource to remain
		in cache.

Loading:
	1. request resource handle by name {
		loader checks vector to see if name has already been requested {
			yes {
				get the stored shared_future<Id_T> and see if it's ready {
					yes {
						see if the Id_T is valid in the cache { -- use the typeId contained in the Id_T to identify the cache index
							is valid { -- resource is still in the cache

							}
							not valid { -- resource has fallen out of the cache, needs to be reloaded

							}
						}
					}
					no { -- already requested but not loaded yet
						return a ResourceHandle containing the existing shared_future<Id_T>
					}
				}
			}
			no {
				create a new shared_future<Id_T> and store it, return a ResourceHandle containing it
			}
		}
	}
	2. request resource by handle {

	}
*/

#endif