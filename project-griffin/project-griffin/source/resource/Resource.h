/**
* @file		Resource.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_H_
#define GRIFFIN_RESOURCE_H_

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
		*	The "_T" indicates a type erasure class.
		*	For details on the type erasure pattern, see the following talks and repo:
		*	https://www.youtube.com/watch?v=qH6sSOr-yk8
		*	https://www.youtube.com/watch?v=bIhUE5uUFOA
		*	https://www.youtube.com/watch?v=0I0FD3N5cgM
		*	https://github.com/tzlaine/type_erasure/
		*/
		class Resource_T {
		public:
			template <typename T>
			explicit Resource_T(T&& x, size_t sizeBytes/*, ResourceCache& cache*/) :
				m_selfPtr(std::make_shared<model<T>>(std::forward<T>(x), sizeBytes/*, cache*/))
			{}

			size_t sizeBytes() const { return m_selfPtr->m_sizeBytes; }

			/**
			* getResource is a potentially type-unsafe operation since the user could call this for
			* a type different than the type used to construct the object. The runtime assert helps
			* to guard against misuse. In general, this is not a polymorphic use of this type, nor
			* is it duck typing. This relies on contextual knowledge that a specific type is
			* contained for safe use.
			*/
			template <typename T>
			T& getResource()
			{
				auto *mdl = reinterpret_cast<model<T>*>(const_cast<concept*>(m_selfPtr.get()));
				
				assert(&typeid(T) == mdl->m_typeId); // check type safety in assert-enabled builds

				return mdl->m_data;
			}

		private:
			Resource_T(const Resource_T&) = delete;

			/**
			* contains functions and variables common to all resources
			*/
			struct concept {
				explicit concept(size_t sizeBytes/*, ResourceCache &cache*/) :
					m_sizeBytes{ sizeBytes }/*,
					m_cache{ cache }*/
				{}

				virtual ~concept() = default;

				size_t m_sizeBytes;
				//ResourceCache&	m_cache;
			};

			/**
			* contains data specific to the resource type T
			*/
			template <typename T>
			struct model : concept {
				explicit model(T&& x, size_t sizeBytes/*, ResourceCache& cache*/) :
					m_data(std::forward<T>(x)),
					m_typeId(&typeid(T)),
					concept(sizeBytes/*, cache*/)
				{}

				T m_data;
				const type_info* m_typeId; // continue to use C++ RTTI or roll my own??
			};

			// pointer to internal model (like PIMPL)
			std::shared_ptr<const concept> m_selfPtr = nullptr;
		};

	}
}

#endif