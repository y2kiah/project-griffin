#include "../ResourceLoader.h"
#include <utility/memory_reserve.h>

namespace griffin {
	namespace resource {

		void ResourceLoader::update(const UpdateInfo& ui)
		{
			std::vector<std::function<void()>> callbacks;
			callbacks.reserve(RESERVE_RESOURCELOADER_CALLBACKS);
			m_callbacks.try_pop_all(callbacks, RESERVE_RESOURCELOADER_CALLBACKS);

			for (auto& cb : callbacks) {
				cb();
			}
		}

		void ResourceLoader::registerCache(const ResourceCachePtr& cachePtr, CacheType cacheTypeId)
		{
			auto f = m_c([=](Impl& impl) {
				impl.m_caches[cacheTypeId] = cachePtr;
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