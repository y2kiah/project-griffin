/**
* @file		ResourceLoader.inl
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_LOADER_INL_H_
#define GRIFFIN_RESOURCE_LOADER_INL_H_

#include <utility/container/handle_map.h>
#include "../ResourceLoader.h"

namespace griffin {
	namespace resource {

		using std::string;

		template <typename T, typename BuilderFunc>
		ResourceHandle<T> ResourceLoader::load(
			const wstring &name,
			CacheType cache_,
			BuilderFunc&& builder,
			CallbackFunc_T callback)
		{
			static_assert(std::is_same<std::result_of<BuilderFunc(DataPtr, size_t)>::type, T>::value, "builder must return an object of type T");

			ResourceHandle<T> h;

			// Can this safely capture name and callback by reference?
			// Or do they need to be copied for the lambda to refer to them later?
			h.resourceId = m_c([=, this](Impl& impl) {
				// check cache for resource first
				auto indexIterator = impl.m_nameToHandle.find(name);
				if (indexIterator != impl.m_nameToHandle.end()) {
					Id_T cacheHandle = indexIterator->second;
					auto& cache = *impl.m_caches[cacheHandle.typeId].get();
					if (cache.hasResource(cacheHandle)) {
						cache.setLRUMostRecent(cacheHandle);
						return cacheHandle;
					}
				}

				// else go to source for resource
				
// TEMP hack, hard coded to first source
				auto* source = impl.m_sources[0].get();
				if (!source->hasResource(name)) {
					string s(name.begin(), name.end());
					throw std::runtime_error(s + ": resource not found in source");
				}

				size_t size = 0;
				auto dataPtr = source->load(name, &size);

				if (!dataPtr || size == 0) {
					string s(name.begin(), name.end());
					throw std::runtime_error(s + ": no data loaded");
				}

				// call the builder functor supplied as a template param to construct and return
				// an object of type T, put it into a shared_ptr
				auto resourcePtr = std::make_shared<Resource_T>(
						builder(std::move(dataPtr), size),
						size/*, impl.m_cache*/);

				// add to the LRU cache, which also puts it at the front
				auto id = impl.m_caches[cache_]->addResource(resourcePtr);
				
				// add handle to index
				impl.m_nameToHandle[name] = id;

				// wrap callback in a closure and queue it to be run on the update thread, this function
				// is running on the resource loader thread
				if (callback) {
					m_callbacks.push([resourcePtr, id, size, callback](){ callback(resourcePtr, id, size); });
				}

				return id;
			});

			return h;
		}


		template <typename T>
		ResourceHandle<T> ResourceLoader::addResourceToCache(ResourcePtr resource, CacheType cache_)
		{
			ResourceHandle<T> handle;

			auto f = m_c([=](Impl& impl) {
				auto& cache = *impl.m_caches[cache_].get();

				return cache.addResource(resource);
			});

			handle.resourceId = f;
			return handle;
		}


		template <typename T>
		std::shared_future<ResourcePtr> ResourceLoader::getResource(const ResourceHandle<T>& h)
		{
			Id_T handle = h.handle();

			auto f = m_c([=](Impl& impl) {
				auto& cache = *impl.m_caches[handle.typeId].get();

				if (!cache.hasResource(handle)) {
					throw std::runtime_error("resource not found by handle");
				}
				return cache.getResource(handle);
			});

			return f;
		}


		template <typename T>
		T* ResourceLoader::getResource(Id_T h, CacheType cache_)
		{
			auto f = m_c([=](Impl& impl) {
				auto& cache = *impl.m_caches[cache_].get();

				if (!cache.hasResource(h)) {
					throw std::runtime_error("resource not found by handle");
				}
				return cache.getResource(h);
			});

			if (f.get().get() == nullptr) {
				return nullptr;
			}
			return static_cast<T*>(&f.get()->getResource<T>());
		}

	}
}

#endif