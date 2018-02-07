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
#include "Resource.h"
#include "ResourceCache.h"
#include "ResourceSource.h"

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
using std::wstring;

namespace griffin {
	namespace resource {

		/**
		* ResourceLoader is a manager of shared resources used by the engine, and provides an
		* asynchronous API for loading, injecting, and getting resources by handle. All functions
		* that deal with ResourceHandle<T> are asynchronous. Some functions are synchronous and
		* deal directly with resource Id_T values.
		*/
		class ResourceLoader {
		public:
			/**
			* Signature: resource shared_ptr, resource handle, resource size
			*/
			typedef std::function<void(const ResourcePtr&, Id_T, size_t)>	CallbackFunc_T;

			explicit ResourceLoader() :
				m_c{}, m_callbacks{}
			{}

			/**
			* executing on each frame currently, not tied to the update fixed timestep
			*/
			void executeCallbacks();

			/**
			* loads a resource from disk (or resource source) into cache, executing the builder
			* callback which must return an object of the resource type T. The builder callback
			* is executed on the loader thread, so do not include calls to OpenGL or other thread-
			* specific resources.
			*/
			template <typename T, typename BuilderFunc>
			ResourceHandle<T> load(const wstring& name, CacheType cache_,
								   BuilderFunc&& builder, CallbackFunc_T callback = nullptr);

			/**
			* Injects a resource already in memory into the resource cache, and returns a resource
			* handle. This allows resources to be manually loaded and then incorporated into the
			* resource cache to be managed and shared. You can optionally give the resource a
			* unique name, so it can be requested by name in other areas of code.
			*/
			template <typename T>
			ResourceHandle<T> addResourceToCache(ResourcePtr resource, CacheType cache_, const wchar_t* name = nullptr);

			/**
			* Asynchronously gets a ResourcePtr by handle.
			*/
			template <typename T>
			std::shared_future<ResourcePtr> getResource(const ResourceHandle<T>& h);

			/**
			* Blocking call, gets a resource directly from the cache. Try to store the ResourcePtr
			* instead of calling this a lot, due to the performance penalty of this call.
			* @return	pointer to resource or nullptr if resource not found
			*/
			ResourcePtr getResource(Id_T h, CacheType cache_);

			/**
			* Blocking call, gets a resource directly from the cache. Try to store the ResourcePtr
			* instead of calling this a lot, due to the performance penalty of this call.
			* @return	pointer to resource or nullptr if resource not found
			*/
			ResourcePtr getResource(const wstring& name, CacheType cache_);

			/**
			* Blocking call, register a cache with the loader. Done during engine init.
			*/
			void registerCache(const ResourceCachePtr& cachePtr, CacheType cache_);
			
			/**
			* Blocking call, register a source with the loader. Done during engine init.
			*/
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
		typedef weak_ptr<ResourceLoader>	ResourceLoaderWeakPtr;
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
	5. allow clients to implement a caching algorithm independent of the internal LRU cache
	6. allow a single container to store many different resource types
	7. don't be intrusive, resource implementers don't inherit from a common base class, prefer
		duck typing over polymorphism

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
					ready {
						see if the Id_T is valid in the cache { -- use the typeId contained in the Id_T to identify the cache index
							is valid { -- resource is still in the cache
								return the ResourceHandle
							}
							not valid { -- resource has fallen out of the cache, needs to be reloaded
								load from source (same as below)
							}
						}
					}
					not ready { -- already requested but not loaded yet
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