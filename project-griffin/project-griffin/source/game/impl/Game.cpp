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
#include <utility/container/handle_map.h>
#include <SDL_log.h>


namespace griffin {
	// Type Definitions

	// Functions

	/**
	* Runs the simulation logic at a fixed frame rate. Keep a "previous" and "next" value for
	* any state that needs to be interpolated smoothly in the renderFrameTick loop. The sceneNode
	* position and orientation are interpolated automatically, but other values like color that
	* need smooth interpolation for rendering should be handled manually.
	*/
	void gameUpdateFrameTick(Game* pGame, Engine& engine, UpdateInfo& ui)
	{
		Game& game = *pGame;
		// if all systems operate on 1(+) frame-old-data, can all systems be run in parallel?
		// should this list become a task flow graph?

		game.player.updateFrameTick(pGame, engine, ui);

		game.devCamera.updateFrameTick(pGame, engine, ui);
	}


	/**
	* Runs at the "full" variable frame rate of the render loop, often bound to vsync at 60hz. For
	* smooth animation, state must be kept from the two most recent update ticks, and interpolated
	* in this loop for final rendering.
	*/
	void gameRenderFrameTick(Game* pGame, Engine& engine, float interpolation)
	{
		Game& game = *pGame;


	}


	/**
	* Create and init the initial game state and game systems and do dependency injection
	*/
	GamePtr make_game(const Engine& engine, const SDLApplication& app)
	{
		GamePtr gamePtr = std::make_shared<Game>();
		Game& game = *gamePtr.get();

		//entity::test_reflection(); // TEMP

		// InputSystem.lua contains initInputSystem function
		engine.scriptManager->doFile(engine.engineLuaState, "scripts/game/initGame.lua"); // throws on error

		// invoke Lua function to init the game
		engine.scriptManager->callLuaGlobalFunction(engine.engineLuaState, "initGame");

		// set up game scene
		{
			using namespace griffin::scene;

			game.sceneId = engine.sceneManager->createScene("Game World", true);
			auto& scene = engine.sceneManager->getScene(game.sceneId);

			game.player.init(gamePtr.get(), engine, app);
			game.devCamera.init(gamePtr.get(), engine, app);
			
			// ...
		}

		// startup active input contexts
		engine.inputSystem->setContextActive(game.player.playerfpsInputContextId);

		return gamePtr;
	}


	/**
	* Called to destroy the systems that should specifically be removed on the OpenGL thread
	*/
	void destroy_game(const GamePtr& gamePtr)
	{
		
	}

}