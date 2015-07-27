#include "../ResourceLoader.h"
#include <utility/memory_reserve.h>

namespace griffin {
	namespace resource {

		void ResourceLoader::update(const UpdateInfo& ui)
		{
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