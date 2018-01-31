#pragma once
#ifndef GRIFFIN_GAME_GAMEIMPL_H_
#define GRIFFIN_GAME_GAMEIMPL_H_

#include <game/Game.h>
#include <utility/container/handle_map.h>
#include <game/player/PlayerControlSystem.h>
#include <game/devCamera/DevCameraSystem.h>
#include <game/devConsole/DevConsoleSystem.h>
#include <game/sky/SkySystem.h>
#include <game/terrain/TerrainSystem.h>
#include <game/positionalEffects/screenShaker/ScreenShakerSystem.h>


//#define MAX_GAME_COMPONENTS		32

namespace griffin {

	//enum : uint16_t {
		//ScreenShakerComponentTypeId
	//};


	/**
	* The Game structure contains all memory for the game state other than data stored in the
	* component store. This is allocated on the heap as a single block and kept with a unique_ptr.
	* Avoid allocating sub-objects on the heap whenever possible, try to keep everything in the
	* game within this block.
	*/
	struct Game {
		Id_T						sceneId = NullId_T;
		game::PlayerControlSystem	player;
		game::SkySystem				sky;
		game::TerrainSystem			terrain;
		game::DevCameraSystem		devCamera;
		game::DevConsoleSystem		devConsole;
		game::ScreenShakerSystem	screenShaker;

		//uint16_t					gameComponentStoreIds[MAX_GAME_COMPONENTS] = {};

		~Game();
	};


}

#endif