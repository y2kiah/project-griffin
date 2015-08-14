#pragma once
#ifndef GRIFFIN_GAME_GAMEIMPL_H_
#define GRIFFIN_GAME_GAMEIMPL_H_

#include <game/Game.h>
#include <game/player/PlayerControlSystem.h>
#include <game/devCamera/DevCameraSystem.h>
#include <utility/container/handle_map.h>


#define GRIFFIN_MAX_GAME_COMPONENTS		32

namespace griffin {

	enum {
		DevCameraMovementComponentTypeId = 0
	};


	/**
	* The Game structure contains all memory for the game state. It is allocated on the heap as a
	* single block and kept with a unique_ptr. Avoid allocating sub-objects on the heap whenever
	* possible, try to keep everything in the game within this block.
	*/
	struct Game {
		Id_T						sceneId;
		game::PlayerControlSystem	player;
		game::DevCameraSystem		devCamera;

		uint16_t					gameComponentStoreIds[GRIFFIN_MAX_GAME_COMPONENTS];
	};


}

#endif