/**
* @file Game.cpp
* @author Jeff Kiah
*/

#include <game/Game.h>
#include <application/Engine.h>
#include <entity/EntityManager.h>
#include <script/ScriptManager_LuaJIT.h>
#include <SDL_log.h>


namespace griffin {
	
	/**
	* Create and init the initial game state and game systems and do dependency injection
	*/
	Game make_game(const Engine& engine, const SDLApplication& app)
	{
		Game game;

		//entity::test_reflection(); // TEMP

		// InputSystem.lua contains initInputSystem function
		engine.scriptManager->doFile(engine.engineLuaState, "scripts/game/initGame.lua"); // throws on error

		// invoke Lua function to init the game
		engine.scriptManager->callLuaGlobalFunction(engine.engineLuaState, "initGame");

		return game;
	}


	/**
	* Called to destroy the systems that should specifically be removed on the OpenGL thread
	*/
	void destroy_game(Game& game)
	{
		
	}
}