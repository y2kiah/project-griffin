#pragma once
#ifndef GRIFFIN_RESOURCE_H
#define GRIFFIN_RESOURCE_H

#include <memory>
#include <future>
#include "ResourceCache.h"

namespace griffin {
	namespace resource {

		/**
		 *
		 */
		template <typename T>
		struct ResourceHandle {
			std::shared_future<Id_T> resourceId;

			bool isAvailable() const {
				#ifdef _MSC_VER
				return resourceId._Is_ready();
				#else
				return (resourceId.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
				#endif
			}

			uint64_t value() const { return resourceId.get().value; }
		};


		/**
		 * @class Resource_T
		 *	For details on the "concept_t" pattern, see Sean Parent's talks "C++ Seasoning" and
		 *	"Inheritance is the base class of evil"
		 *	https://www.youtube.com/watch?v=qH6sSOr-yk8
		 *	https://www.youtube.com/watch?v=bIhUE5uUFOA
		 */
		class Resource_T {
		public:
			template <typename T>
			Resource_T(T&& x, size_t sizeBytes, ResourceCache& cache) :
				m_selfPtr(std::make_shared<model<T>>(std::forward<T>(x), sizeBytes, cache))
			{}

			size_t sizeBytes() const { return m_selfPtr->m_sizeBytes; }

			template <typename T>
			T& getResource()
			{
				model<T> *mdl = (model<T>*)m_selfPtr.get();
				return mdl->m_data;
			}

		private:
			/**
			 * contains functions and variables common to all resources
			 */
			struct concept_T {
				explicit concept_T(size_t sizeBytes, ResourceCache &cache) :
					m_sizeBytes{ sizeBytes },
					m_cache{ cache }
				{}

				virtual ~concept_T()
				{
					// tell cache that this resource's memory has been freed
					m_cache.memoryHasBeenFreed(m_sizeBytes);
				}

				size_t			m_sizeBytes;
				ResourceCache&	m_cache;
			};

			/**
			 * contains data specific to the resource type T
			 */
			template <typename T>
			struct model : concept_T {
				explicit model(T&& x, size_t sizeBytes, ResourceCache& cache) :
					m_data(std::forward<T>(x)),
					concept_T(sizeBytes, cache)
				{}

				T m_data;
			};

			// pointer to internal implementation (PIMPL)
			std::shared_ptr<const concept_T> m_selfPtr;
		};

	}
}

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