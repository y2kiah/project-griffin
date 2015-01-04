#pragma once
#ifndef GRIFFIN_RESOURCE_LOADER_H
#define GRIFFIN_RESOURCE_LOADER_H

#include <string>
#include <memory>
#include <future>
#include <functional>
#include <cvt/wstring>
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
				m_c({ std::forward<ResourceCache>(cache), source })
			{}

			/**
			 *
			 */
			template <typename T>
			ResourceHandle<T> load(const wstring &name, std::function<void(T&)> callback = nullptr);

			/**
			 *
			 */
			template <typename T>
			T& getResource(ResourceHandle<T>& inOutHandle)
			{
				//if (m_cache. inOutHandle) {}
			}

		private:
			struct Impl {
				ResourceCache		m_cache;
				IResourceSource	*	m_source;
				std::vector<std::pair<std::wstring, Id_T>> m_nameToHandle;
			};
			concurrent<Impl> m_c;
		};

	}
}

#include "impl/ResourceLoader.inl"

#endif