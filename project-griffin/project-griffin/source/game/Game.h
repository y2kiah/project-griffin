/**
* @file Game.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_GAME_H_
#define GRIFFIN_GAME_H_

#include "GameSystem.h"


// Forward Declarations
class SDLApplication;


namespace griffin {
	struct Engine;


	struct Game {
		

	};

	Game make_game(const Engine& engine, const SDLApplication& app);
	void destroy_game(Game& game);

}

#endif