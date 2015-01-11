#include "../Application.h"
#include <SDL.h>

// TEMP
#include <render/Render.h>
#include <render/texture/Texture2D_GL.h>

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

		auto stringResourceBuilder = [](DataPtr&& data, size_t size) {
			std::string r(reinterpret_cast<char*>(data.get()));
			SDL_Log("building resource:\n%s", r.c_str());
			return r;
		};

		/// TEXTURE LOADING
		auto textureResourceBuilder = [](DataPtr&& data, size_t size) {
			Texture2D_GL tex(std::move(data), size);
			return std::move(tex);
		};
		auto textureResourceCallback = [](const ResourcePtr& resourcePtr, Id_T handle, size_t size) {
			Texture2D_GL& tex = resourcePtr->getResource<Texture2D_GL>();
			// the unique_ptr of data is stored within the texture, this call deletes the data after
			// sending texture to OpenGL
			tex.loadFromInternalMemory();
		};

		auto handle3 = loader.load<Texture2D_GL>(L"../vendor/soil/img_test.png", textureResourceBuilder, textureResourceCallback);
		///

		auto handle1 = loader.load<string>(L"shaders/SimpleVertexShader.glsl", stringResourceBuilder);
		auto handle2 = loader.load<string>(L"shaders/SimpleFragmentShader.glsl", stringResourceBuilder);

		handle3.resourceId.wait();
		loader.executeCallbacks();

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

			SDL_Log("Id3 = %llu", handle3.value());
			SDL_Log("index3 = %u", handle3.resourceId.get().index);
			SDL_Log("typeid3 = %u", handle3.resourceId.get().typeId);
			SDL_Log("gen3 = %u", handle3.resourceId.get().generation);
			SDL_Log("free3 = %u", handle3.resourceId.get().free);
		}
		catch (std::runtime_error& ex) {
			SDL_Log(ex.what());
		}
	}

}