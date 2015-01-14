#include "../Application.h"
#include <SDL.h>

// TEMP
#include <render/Render.h>

namespace griffin {

	using std::make_unique;
	using std::make_shared;
	using std::move;

	Application make_application()
	{
		Application app;

		/**
		* Build the Resource System
		*/
		{
			using namespace resource;

			auto loaderPtr = make_shared<ResourceLoader>();

			// Build resource caches
			auto materialsCachePtr = make_shared<ResourceCache>(Cache_Materials_T, 10, 1024 * 1024 /* 1 MB */);
			loaderPtr->registerCache(materialsCachePtr, (CacheType)materialsCachePtr->getItemTypeId());

			// Build resource sources
			auto fileSystemSourcePtr = IResourceSourcePtr((IResourceSource*)(new FileSystemSource()));
			loaderPtr->registerSource(fileSystemSourcePtr);

			// inject dependencies to the loader
			render::g_loaderPtr = loaderPtr;

			app.resourceLoader = loaderPtr;
		}
		
		return move(app);
	}

}