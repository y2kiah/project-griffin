#pragma once
#ifndef GRIFFIN_RESOURCE_H
#define GRIFFIN_RESOURCE_H

#include <memory>
#include <utility/container/handle_map.h>
#include <utility/concurrency.h>
#include <utility/enum.h>
#include <future>

namespace griffin {
	namespace resource {

		MakeEnum(ResourceTypes, uint8_t,
				(Texture)
				(Mesh)
				(Shader)
			, _TypeId);


		/**
		 *
		 */
		struct ResourceHandle {
			std::shared_future<Id_T> resourceId;

			bool isAvailable() const {
				#ifdef _MSC_VER
				return resourceId._Is_ready();
				#else
				return (resourceId.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
				#endif
			}
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
			Resource_T(T x) : m_selfPtr(std::make_shared<model<T>>(std::move(x))) {}

			size_t sizeBytes() const { return m_sizeBytes; }

		private:
			struct concept_T {
				virtual ~concept_T() = default;
				//virtual size_t sizeBytes() const = 0;
			};

			template<typename T>
			class model : concept_T {
			public:
				model(T&& x) : m_data(std::forward<T>(x)) {}
				//size_t sizeBytes() const { return m_sizeBytes; }

				T m_data;
			};

			std::shared_ptr<const concept_T> m_selfPtr;
			size_t m_sizeBytes;
		};


		/**
		 *
		 */
		class ResourceCache {
			//typedef handle_map<ResourcePtr> m_resourceCache;
		};


		/**
		 *
		 */
		class ResourceLoader {
		public:
			template<typename T>
			ResourceHandle load(
					const wstring &name,
					std::function<void(const T&)> callback)
			{
				ResourceHandle h;
				
				auto f = m_c([=, &name](){
					return 0;
				});

				h.resourceId = f.share();
				return h;
			}

		private:
			struct Impl {



			};
			concurrent<Impl> m_c;
		};
	}
}

#endif