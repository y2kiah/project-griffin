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

	void test_resource_loader(Application& app)
	{
		using namespace resource;
		using namespace render;
		using std::string;

		auto& loader = *app.resourceLoader.get();

		auto stringResourceBuilder = [](DataPtr data, size_t size) {
			std::string r(reinterpret_cast<char*>(data.get()), size);
			SDL_Log("building resource:\n%s", r.c_str());
			return r;
		};

		auto handle1 = loader.load<string>(L"shaders/SimpleVertexShader.glsl", stringResourceBuilder);
		auto handle2 = loader.load<string>(L"shaders/SimpleFragmentShader.glsl", stringResourceBuilder);

		try {
			SDL_Log("Id1 = %llu", handle1.value());
			SDL_Log("index1 = %u", handle1.resourceId.get().index);
			SDL_Log("typeid1 = %u", handle1.resourceId.get().typeId);
			SDL_Log("gen1 = %u", handle1.resourceId.get().generation);
			SDL_Log("free1 = %u", handle1.resourceId.get().free);

			SDL_Log("Id2 = %llu", handle2.value());
			SDL_Log("index2 = %u", handle2.resourceId.get().index);
			SDL_Log("typeid2 = %u", handle2.resourceId.get().typeId);
			SDL_Log("gen2 = %u", handle2.resourceId.get().generation);
			SDL_Log("free2 = %u", handle2.resourceId.get().free);
		}
		catch (std::runtime_error& ex) {
			SDL_Log(ex.what());
		}
	}

}