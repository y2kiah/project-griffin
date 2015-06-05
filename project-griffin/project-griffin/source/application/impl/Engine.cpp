#include "../Engine.h"
#include <SDL.h>

// TEMP
#include <render/Render.h>


using std::make_unique;
using std::make_shared;
using std::move;

namespace griffin {

	Engine make_engine(const SDLApplication& app)
	{
		Engine engine;

		auto scriptPtr = make_shared<script::ScriptManager>();
		auto inputPtr  = make_shared<core::InputSystem>();
		auto loaderPtr = make_shared<resource::ResourceLoader>();

		/**
		* Build the Lua scripting system
		*/
		Id_T initLuaStateId{};
		{
			using namespace script;

			// init.lua configures the startup settings
			initLuaStateId = scriptPtr->createState("scripts/init.lua"); // throws on error

			// add system API functions to Lua


			engine.scriptManager = scriptPtr;
		}

		/**
		* Build the input system
		*/
		{
			using namespace core;

			// inject dependencies into the InputSystem
			inputPtr->app = &app;

			inputPtr->initialize();

			// inject dependencies to the InputSystem C API
			setInputSystemPtr(inputPtr);

			// InputSystem.lua contains initInputSystem function
			scriptPtr->doFile(initLuaStateId, "scripts/InputSystem.lua"); // throws on error

			// invoke Lua function to init InputSystem
			scriptPtr->callLuaGlobalFunction(initLuaStateId, "initInputSystem");

			// move input system into application
			engine.inputSystem = inputPtr;
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

			// inject dependencies into the loader
			render::g_loaderPtr = loaderPtr;

			// add Lua APIs


			engine.resourceLoader = loaderPtr;
		}

		/**
		* Build the RenderSystem
		*/
		{
			using namespace render;

			auto renderSystemPtr = make_shared<RenderSystem>();
			renderSystemPtr->init(app.getPrimaryWindow().width, app.getPrimaryWindow().height);

			engine.renderSystem = renderSystemPtr;
		}

		/**
		* Build the entity-component system
		*/
		{
			using namespace entity;

			auto entityPtr = make_shared<EntityManager>();

			// add Lua APIs


			engine.entityManager = entityPtr;
		}

		#ifdef GRIFFIN_TOOLS_BUILD
		{
			using namespace tools;

			Id_T toolsLuaStateId = scriptPtr->createState();

			auto toolsPtr = make_shared<GriffinToolsManager>();
			//"scripts/tools/toolsServer.lua"

			engine.toolsManager = toolsPtr;
		}
		#endif

		/**
		* Build the systems list for ordered updates
		*/
		{
			// create vector in order of system update execution
			engine.systems.push_back(engine.inputSystem.get());
			//engine.systems.push_back(engine.resourceLoader.get());
		}

		return move(engine);
	}

}