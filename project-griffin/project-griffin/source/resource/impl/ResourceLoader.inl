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
				// else go to source for resource
				if (!impl.m_source->hasResource(name)) {
					// if can't get resource throw exception
				}

				size_t size = 0;
				auto dataPtr = impl.m_source->load(name, &size);

				if (!dataPtr || size == 0) {
					std::string s(name.begin(), name.end());
					throw std::runtime_error(s + ": no data loaded");
				}

				auto resourcePtr = std::make_shared<Resource_T>(T(dataPtr.get()), size, impl.m_cache);

				auto id = impl.m_cache.addResource(resourcePtr, name);

				// if the resource was obtained, tell the LRU cache to put it to front

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


	}
}

#endif