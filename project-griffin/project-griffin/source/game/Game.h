/**
* @file Game.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_GAME_H_
#define GRIFFIN_GAME_H_

#include "GameSystem.h"
#include <utility/container/handle_map.h>
#include <memory>
//#include <scene/Camera.h>


#define MAX_GAME_COMPONENTS		32

// Forward Declarations
class SDLApplication;

namespace griffin {
	struct Engine;

	struct Game {
		Id_T		sceneId;

		struct {
			Id_T	devCameraId;													//<! entity id of the dev camera
			Id_T	devCameraMovementId;											//<! dev camera movement component id

			Id_T	devCameraInputContextId;										//<! input context handle for the devcamera
			Id_T	forward, back, left, right, up, down, highSpeed, lookX, lookY;	//<! mapping handles for the inputs

		} devCameraSystem;
		

		uint16_t	gameComponentStoreIds[MAX_GAME_COMPONENTS];
	};

	typedef std::unique_ptr<Game>	GamePtr;

	GamePtr make_game(const Engine& engine, const SDLApplication& app);
	void destroy_game(GamePtr& gamePtr);

}

#endif