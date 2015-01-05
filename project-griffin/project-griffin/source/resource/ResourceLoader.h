#pragma once
#ifndef GRIFFIN_RESOURCE_LOADER_H
#define GRIFFIN_RESOURCE_LOADER_H

#include <string>
#include <memory>
#include <future>
#include <functional>
#include <boost/container/flat_map.hpp>
#include <utility/concurrency.h>
#include "Resource.h"
#include "ResourceCache.h"
#include "ResourceSource.h"

namespace griffin {
	namespace resource {

		/**
		 *
		 */
		class ResourceLoader {
		public:
			explicit ResourceLoader(ResourceCache&& cache, IResourceSource* source) :
				m_c({ std::forward<ResourceCache>(cache), source, {} })
			{
				// we need a way to call "reserve" on the ResourceNameIndex flat_map, maybe in a Impl constructor?
			}

			/**
			 *
			 */
			template <typename T>
			ResourceHandle<T> load(const wstring &name, std::function<void(T&)> callback = nullptr);

			/**
			 *
			 */
			template <typename T>
			T& getResource(ResourceHandle<T>& inOutHandle);

		private:
			typedef boost::container::flat_map<std::wstring, Id_T> ResourceNameIndex;
			struct Impl {
				ResourceCache		m_cache;
				IResourceSource	*	m_source;
				
				ResourceNameIndex	m_nameToHandle;
			};

			concurrent<Impl> m_c;
		};

	}
}

#include "impl/ResourceLoader.inl"

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
	7. don't be instrusive, resource implementers don't inherit from a common base class, prefer
		duck typing over polymorphism

Myths (to dispell):
	1. using shared_ptr is a performance concern, causes synchronization - copies are rare,
		synchronization only occurs when banging on the same shared_ptr at the same time, which is
		rarely or never

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