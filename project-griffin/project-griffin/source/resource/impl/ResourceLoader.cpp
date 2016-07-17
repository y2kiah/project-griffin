#include "../ResourceLoader.h"
#include <utility/memory_reserve.h>

namespace griffin {
	namespace resource {

		ResourcePtr ResourceLoader::getResource(Id_T h, CacheType cache_)
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
			return f.get();
		}


		ResourcePtr ResourceLoader::getResource(const wstring& name, CacheType cache_)
		{
			auto f = m_c([=](Impl& impl) {
				auto iter = impl.m_nameToHandle.find(name);
				if (iter == impl.m_nameToHandle.end()) {
					throw std::runtime_error("resource not found by handle");
				}

				Id_T h = iter->second;
				auto& cache = *impl.m_caches[cache_].get();

				if (!cache.hasResource(h)) {
					throw std::runtime_error("resource not found by handle");
				}
				return cache.getResource(h);
			});

			if (f.get().get() == nullptr) {
				return nullptr;
			}
			return f.get();
		}


		void ResourceLoader::executeCallbacks()
		{
			std::vector<std::function<void()>> callbacks;
			callbacks.reserve(RESERVE_RESOURCELOADER_CALLBACKS);
			m_callbacks.try_pop_all(callbacks, RESERVE_RESOURCELOADER_CALLBACKS);

			for (auto& cb : callbacks) {
				cb();
			}
		}

		void ResourceLoader::registerCache(const ResourceCachePtr& cachePtr, CacheType cache_)
		{
			auto f = m_c([=](Impl& impl) {
				impl.m_caches[cache_] = cachePtr;
			});
			f.wait();
		}

		void ResourceLoader::registerSource(const IResourceSourcePtr& sourcePtr)
		{
			auto f = m_c([=](Impl& impl) {
				impl.m_sources.push_back(sourcePtr);
			});
			f.wait();
		}

	}
}