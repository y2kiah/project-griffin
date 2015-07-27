/**
* @file Engine.cpp
* @author Jeff Kiah
*/
#include <application/Engine.h>
#include <SDL.h>

using std::make_unique;
using std::make_shared;
using std::move;

namespace griffin {

	ThreadPoolPtr task_base::s_threadPool = nullptr;


	Engine make_engine(const SDLApplication& app)
	{
		Engine engine;

		/**
		* Create thread pool, one worker thread per logical core
		*/
		{
			engine.threadPool = make_shared<thread_pool>(app.getSystemInfo().cpuCount);
			task_base::s_threadPool = engine.threadPool;
		}

		auto scriptPtr  = make_shared<script::ScriptManager>();
		auto inputPtr   = make_shared<core::InputSystem>();
		auto loaderPtr  = make_shared<resource::ResourceLoader>();

		/**
		* Build the Lua scripting system
		*/
		Id_T initLuaStateId{};
		{
			using namespace script;

			// init.lua configures the startup settings
			initLuaStateId = scriptPtr->createState("scripts/initState.lua"); // throws on error

			// add system API functions to Lua


			engine.scriptManager = scriptPtr;
		}

		/**
		* Build the tools manager, GRIFFIN_TOOLS_BUILD only
		*/
		#ifdef GRIFFIN_TOOLS_BUILD
		{
			using namespace tools;

			Id_T toolsLuaStateId = scriptPtr->createState("scripts/initState.lua");

			auto toolsPtr = make_shared<GriffinToolsManager>(toolsLuaStateId);

			// executes tools build scripts, and starts http server on a new thread
			toolsPtr->init(scriptPtr);

			engine.toolsManager = toolsPtr;
		}
		#endif

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
			//   Materials Cache
			auto materialsCachePtr = make_shared<ResourceCache>(Cache_Materials_T, RESERVE_RESOURCECACHE_MATERIALS, 10 * 1024 * 1024 /* 10 MB */);
			loaderPtr->registerCache(materialsCachePtr, (CacheType)materialsCachePtr->getItemTypeId());
			
			//   Scripts Cache
			auto scriptsCachePtr = make_shared<ResourceCache>(Cache_Scripts_T, RESERVE_RESOURCECACHE_SCRIPTS, 1024 * 1024 /* 1 MB */);
			loaderPtr->registerCache(scriptsCachePtr, (CacheType)scriptsCachePtr->getItemTypeId());

			// Build resource sources
			auto fileSystemSourcePtr = IResourceSourcePtr((IResourceSource*)(new FileSystemSource()));
			loaderPtr->registerSource(fileSystemSourcePtr);

			// inject loader dependencies into other system
			render::g_resourceLoader = loaderPtr;

			// add Lua APIs


			engine.resourceLoader = loaderPtr;
		}

		/**
		* Build the render system
		*/
		{
			using namespace render;

			auto renderSystemPtr = make_shared<RenderSystem>();
			renderSystemPtr->init(app.getPrimaryWindow().width, app.getPrimaryWindow().height);

			engine.renderSystem = renderSystemPtr;
		}

		/**
		* Build the scene manager
		*/
		{
			using namespace scene;

			auto scenePtr = make_shared<SceneManager>();

			// inject dependencies to the Scene C API
			setSceneManagerPtr(scenePtr);

			// Scene.lua contains scene functions
			scriptPtr->doFile(initLuaStateId, "scripts/Scene.lua"); // throws on error

			engine.sceneManager = scenePtr;
		}

		/**
		* Build the systems list for ordered updates
		*/
		{
			// create vector in order of system update execution
			engine.systems.push_back(engine.inputSystem.get());
			//engine.systems.push_back(engine.resourceLoader.get());

			engine.systems.push_back(engine.toolsManager.get());
		}

		return move(engine);
	}

}