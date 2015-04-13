#include "../Application.h"
#include <SDL.h>

// TEMP
#include <render/Render.h>


using std::make_unique;
using std::make_shared;
using std::move;

namespace griffin {

	Application make_application()
	{
		Application app;

		auto scriptPtr = make_shared<script::ScriptManager>();
		auto inputPtr  = make_shared<core::InputSystem>();
		auto loaderPtr = make_shared<resource::ResourceLoader>();

		/**
		* Build the Lua scripting system
		*/
		{
			using namespace script;

			// init.lua configures the startup settings
			scriptPtr->init("scripts/init.lua"); // throws on error

			// add system API functions to Lua


			app.scriptManager = scriptPtr;
		}

		/**
		* Build the input system
		*/
		{
			using namespace core;

			inputPtr->initialize();

			// inject dependencies to the InputSystem C API
			setInputSystemPtr(inputPtr);

			// InputSystem.lua contains initInputSystem function
			scriptPtr->doFile("scripts/InputSystem.lua"); // throws on error

			// invoke Lua function to init InputSystem
			scriptPtr->callLuaGlobalFunction("initInputSystem");

			// move input system into application
			app.inputSystem = inputPtr;
		}

		/**
		* Build the resource system
		*/
		{
			using namespace resource;

			// Build resource caches
			auto materialsCachePtr = make_shared<ResourceCache>(Cache_Materials_T, 10, 1024 * 1024 /* 1 MB */);
			loaderPtr->registerCache(materialsCachePtr, (CacheType)materialsCachePtr->getItemTypeId());

			// Build resource sources
			auto fileSystemSourcePtr = IResourceSourcePtr((IResourceSource*)(new FileSystemSource()));
			loaderPtr->registerSource(fileSystemSourcePtr);

			// inject dependencies to the loader
			render::g_loaderPtr = loaderPtr;

			// add Lua APIs


			app.resourceLoader = loaderPtr;
		}

		/**
		* Build the entity-component system
		*/
		{
			using namespace entity;

			auto entityPtr = make_shared<EntityManager>();

			// add Lua APIs


			app.entityManager = entityPtr;
		}

		/**
		* Build the systems list for ordered updates
		*/
		{
			// create vector in order of system update execution
			app.systems.push_back(app.inputSystem.get());

		}

		return move(app);
	}

}