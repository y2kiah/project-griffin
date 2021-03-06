/**
* @file Game.cpp
* @author Jeff Kiah
*/

#include <game/Game.h>
#include <game/impl/GameImpl.h>
#include <application/Engine.h>
#include <scene/Scene.h>
#include <entity/EntityManager.h>
#include <script/ScriptManager_LuaJIT.h>
#include <input/InputSystem.h>
#include <render/Render.h>
#include <utility/container/handle_map.h>


namespace griffin {
	// TEMP
	namespace render {
		extern Game* g_pGame;
	}

	// Type Definitions

	// Functions

	/**
	* Runs the simulation logic at a fixed frame rate. Keep a "previous" and "next" value for
	* any state that needs to be interpolated smoothly in the renderFrameTick loop. The sceneNode
	* position and orientation are interpolated automatically, but other values like color that
	* need smooth interpolation for rendering should be handled manually.
	*/
	void gameUpdateFrameTick(Game& game, Engine& engine, UpdateInfo& ui)
	{
		// if all systems operate on 1(+) frame-old-data, can all systems be run in parallel?
		// should this list become a task flow graph?

		game.player.updateFrameTick(game, engine, ui);

		game.devCamera.updateFrameTick(game, engine, ui);

		game.terrain.updateFrameTick(game, engine, ui);

		// TODO: consider running this less frequently, and spread the load with other systems that
		//	don't run every frame by offsetting the frame that it runs on
		game.sky.updateFrameTick(game, engine, ui);

		game.screenShaker.updateFrameTick(game, engine, ui);
	}


	/**
	* Runs at the "full" variable frame rate of the render loop, often bound to vsync at 60hz. For
	* smooth animation, state must be kept from the two most recent update ticks, and interpolated
	* in this loop for final rendering.
	*/
	void gameRenderFrameTick(Game& game, Engine& engine, float interpolation,
							 const int64_t realTime, const int64_t countsPassed)
	{
		game.screenShaker.renderFrameTick(game, engine, interpolation, realTime, countsPassed);

		game.devConsole.renderFrameTick(game, engine, interpolation, realTime, countsPassed);
	}


	/**
	* Create and init the initial game state and game systems and do dependency injection
	*/
	GamePtr make_game(const Engine& engine, const SDLApplication& app)
	{
		GamePtr gamePtr = std::make_shared<Game>();
		Game& game = *gamePtr;

		// InputSystem.lua contains initInputSystem function
		engine.scriptManager->doFile(engine.engineLuaState, "scripts/game/initGame.lua"); // throws on error

		// invoke Lua function to init the game
		engine.scriptManager->callLuaGlobalFunction(engine.engineLuaState, "initGame");

		// set up game scene
		{
			using namespace griffin::scene;

			game.sceneId = engine.sceneManager->createScene("Game World", true);
			auto& scene = engine.sceneManager->getScene(game.sceneId);
			
			// set up terrain system
			game.terrain.init(game, engine, app);
			render::g_pGame = gamePtr.get(); // TEMP

			// set up sky system
			game.sky.init(game, engine, app);
			// TODO: scene should have a setSkybox convenience function or something like that
			//scene.setSkybox(game.sky.skyBoxCubeMap);
			// temp, will be part of scene as above
			engine.renderSystem->setSkyboxTexture(game.sky.skyBoxCubeMap);

			// set up input control systems
			game.player.init(game, engine, app);
			game.devCamera.init(game, engine, app);

			// set up development tools
			game.devConsole.init(game, engine, app);
			
			// set up positionalEffect systems
			game.screenShaker.init(game, engine, app);

			// ...
		}

		// startup active input contexts
		engine.inputSystem->setContextActive(game.player.playerfpsInputContextId);

		return gamePtr;
	}


	/**
	* Destructor releases all systems on the OpenGL thread
	*/
	Game::~Game()
	{
		terrain.deinit();
	}

}