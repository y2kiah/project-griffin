#pragma once
#ifndef GRIFFIN_RESOURCE_LOADER_INL
#define GRIFFIN_RESOURCE_LOADER_INL

#include <utility/container/handle_map.h>
#include "../ResourceLoader.h"

namespace griffin {
	namespace resource {

		template <typename T>
		ResourceHandle<T> ResourceLoader::load(
			const wstring &name,
			std::function<void(T&)> callback)
		{
			ResourceHandle<T> h;

			// Can this safely capture name and callback by reference?
			// Or do they need to be copied for the lambda to refer to them later?
			auto f = m_c([=](Impl& impl) {
				// check cache for resource first
				auto indexIterator = impl.m_nameToHandle.find(name);
				if (indexIterator != impl.m_nameToHandle.end()) {
					Id_T cacheHandle = indexIterator->second;
					if (impl.m_cache.hasResource(cacheHandle)) {
						impl.m_cache.setLRUMostRecent(cacheHandle);
						return cacheHandle;
					}
				}

				// else go to source for resource
				if (!impl.m_source->hasResource(name)) {
					std::string s(name.begin(), name.end());
					throw std::runtime_error(s + ": resource not found in source");
				}

				size_t size = 0;
				auto dataPtr = impl.m_source->load(name, &size);

				if (!dataPtr || size == 0) {
					std::string s(name.begin(), name.end());
					throw std::runtime_error(s + ": no data loaded");
				}

				// this needs to be looked at, the dataPtr should be moved into the constructor so
				// no copying of the data takes place, the existing data should be used in place
				// could call dataPtr.release() if the resource T is placement-new'd, but that is
				// an intrusive implementation requirement. Really should think about breaking out
				// this construction responsibility into a lambda that is passed in (like the
				// callback)
				auto resourcePtr = std::make_shared<Resource_T>(T(dataPtr.get()), size/*, impl.m_cache*/);

				// add to the LRU cache, which also puts it at the front
				auto id = impl.m_cache.addResource(resourcePtr);
				
				// add handle to index
				impl.m_nameToHandle[name] = id;

				// don't call this here, it will run the callback on the resource loader thread
				// instead wrap closure around the callback (binding the resource value) and
				// add to a callback queue to be called once per frame from the update thread
				if (callback) {
					callback(resourcePtr->getResource<T>());
				}

				return id;
			});

			h.resourceId = f.share();
			return h;
		}

		template <typename T>
		std::future<ResourcePtr> ResourceLoader::getResource(ResourceHandle<T>& inOutHandle)
		{
			auto f = m_c([=](Impl& impl) {
				Id_T handle = inOutHandle.handle();
				if (!impl.m_cache.hasResource(handle)) {
					throw std::runtime_error("resource not found by handle");
				}
				return impl.m_cache.getResource(handle);
			});

			return f;
		}

	}
}

#endif