/**
* @file Engine.cpp
* @author Jeff Kiah
*/
#include <application/Engine.h>
#include <input/InputSystem.h>
#include <resource/ResourceLoader.h>
#include <script/ScriptManager_LuaJIT.h>
#include <render/Render.h>
#include <scene/Scene.h>
#include <tools/GriffinTools.h>
#include <SDL.h>

using std::make_unique;
using std::make_shared;
using std::move;

namespace griffin {

	ThreadPoolPtr task_base::s_threadPool = nullptr;

	/**
	*
	*/
	void engineUpdateFrameTick(Engine& engine, UpdateInfo& ui)
	{
		// if all systems operate on 1(+) frame-old-data, can all systems be run in parallel?
		// should this list become a task flow graph?

		engine.inputSystem->updateFrameTick(ui);

		// call the Lua game update handler

		//	ResourceLoader
		//	AISystem
		//	PhysicsSystem
		//	CollisionSystem
		//	ResourcePredictionSystem
		//	etc.

		engine.sceneManager->updateActiveScenes();

		// below currently does nothing
		//#ifdef GRIFFIN_TOOLS_BUILD
		//engine.toolsManager->updateFrameTick(ui);
		//#endif
	}


	/**
	*
	*/
	void engineRenderFrameTick(Engine& engine, float interpolation)
	{
		engine.resourceLoader->executeCallbacks();

		engine.renderSystem->renderFrame(interpolation);
	}


	/**
	* Create and init the systems of the griffin engine, and do dependency injection
	*/
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
		auto inputPtr   = make_shared<input::InputSystem>();
		auto loaderPtr  = make_shared<resource::ResourceLoader>();

		/**
		* Build the Lua scripting system
		*/
		{
			using namespace script;

			// init.lua configures the startup settings
			engine.engineLuaState = scriptPtr->createState("scripts/initState.lua"); // throws on error

			// add system API functions to Lua


			engine.scriptManager = scriptPtr;
		}

		/**
		* Build the tools manager, GRIFFIN_TOOLS_BUILD only
		*/
		#ifdef GRIFFIN_TOOLS_BUILD
		{
			using namespace tools;

			engine.toolsLuaState = scriptPtr->createState("scripts/initState.lua");

			auto toolsPtr = make_shared<GriffinToolsManager>(engine.toolsLuaState);

			// executes tools build scripts, and starts http server on a new thread
			toolsPtr->init(scriptPtr);

			engine.toolsManager = toolsPtr;
		}
		#endif

		/**
		* Build the input system
		*/
		{
			using namespace input;

			// inject dependencies into the InputSystem
			inputPtr->app = &app;

			inputPtr->initialize();

			// inject dependencies to the InputSystem C API
			setInputSystemPtr(inputPtr);

			// InputSystem.lua contains initInputSystem function
			scriptPtr->doFile(engine.engineLuaState, "scripts/InputSystem.lua"); // throws on error

			// invoke Lua function to init InputSystem
			scriptPtr->callLuaGlobalFunction(engine.engineLuaState, "initInputSystem");

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
			#ifdef GRIFFIN_TOOLS_BUILD
			tools::setResourceLoaderPtr(loaderPtr);
			#endif

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
			scriptPtr->doFile(engine.engineLuaState, "scripts/Scene.lua"); // throws on error

			engine.sceneManager = scenePtr;
		}

		return engine;
	}


	/**
	* Called to destroy the systems that should specifically be removed on the OpenGL thread
	*/
	void destroy_engine(Engine& engine)
	{
		// Destroy the tools system
		#ifdef GRIFFIN_TOOLS_BUILD
		engine.toolsManager.reset();
		#endif

		// Destroy the render system
		engine.renderSystem.reset();

		// Destroy the resource system
		engine.resourceLoader.reset();
	}
}