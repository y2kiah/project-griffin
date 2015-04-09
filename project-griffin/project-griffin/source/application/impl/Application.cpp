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

		/**
		* Build the Lua scripting system
		*/
		{
			using namespace script;

			auto scriptPtr = make_shared<ScriptManager>();

			// init.lua configures the startup settings
			if (!scriptPtr->init("scripts/init.lua")) {
				throw std::runtime_error("scripts/init.lua not found");
			}
			// config.lua loads configuration files
			if (!scriptPtr->doFile("scripts/config.lua")) {
				throw std::runtime_error("scripts/config.lua not found");
			}

			// add system API functions to Lua
			
			// call Lua function to load config
			scriptPtr->callLuaGlobalFunction("loadInputSystemConfig"); // TEMP

			app.scriptManager = scriptPtr;
		}

		/**
		* Build the input system
		*/
		{
			using namespace core;

			auto inputPtr = make_shared<InputSystem>();
			inputPtr->initialize();

			// add Lua APIs

			// invoke Lua function to load input config


			// move input system into application
			app.inputSystem = inputPtr;
		}

		/**
		* Build the resource system
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