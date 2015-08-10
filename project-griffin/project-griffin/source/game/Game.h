/**
* @file Game.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_GAME_H_
#define GRIFFIN_GAME_H_

#include <memory>
#include <application/UpdateInfo.h>


// Forward Declarations
class SDLApplication;

namespace griffin {
	struct Engine;
	struct Game;

	typedef std::shared_ptr<Game>	GamePtr;

	void gameUpdateFrameTick(Game* pGame, Engine& engine, UpdateInfo& ui);
	void gameRenderFrameTick(Game* pGame, Engine& engine, float interpolation);

	GamePtr make_game(const Engine& engine, const SDLApplication& app);
	void destroy_game(const GamePtr& gamePtr);

}

#endif