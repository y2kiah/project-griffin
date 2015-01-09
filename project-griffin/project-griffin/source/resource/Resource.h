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

			// both are blocking calls, surround with a check of isAvailable to poll for result
			Id_T handle() const { return resourceId.get(); }
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
			Resource_T(T&& x, size_t sizeBytes/*, ResourceCache& cache*/) :
				m_selfPtr(std::make_shared<model<T>>(std::forward<T>(x), sizeBytes/*, cache*/))
			{}

			size_t sizeBytes() const { return m_selfPtr->m_sizeBytes; }

			template <typename T>
			T& getResource()
			{
				assert(&typeid(T) == m_selfPtr->m_typeId); // check type safety in assert-enabled builds

				auto *mdl = (model<T>*)m_selfPtr.get();
				return mdl->m_data;
			}

		private:
			/**
			 * contains functions and variables common to all resources
			 */
			struct concept_T {
				explicit concept_T(size_t sizeBytes/*, ResourceCache &cache*/) :
					m_sizeBytes{ sizeBytes }/*,
					m_cache{ cache }*/
				{}

				virtual ~concept_T() = default;

				size_t m_sizeBytes;
				//ResourceCache&	m_cache;
			};

			/**
			 * contains data specific to the resource type T
			 */
			template <typename T>
			struct model : concept_T {
				explicit model(T&& x, size_t sizeBytes/*, ResourceCache& cache*/) :
					m_data(std::forward<T>(x)),
					m_typeId(&typeid(T)),
					concept_T(sizeBytes/*, cache*/)
				{}

				T m_data;
				const type_info* m_typeId;
			};

			// pointer to internal model (like PIMPL)
			std::shared_ptr<const concept_T> m_selfPtr;
		};

	}
}

#endif