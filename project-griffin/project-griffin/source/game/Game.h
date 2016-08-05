/**
* @file Game.h
* @author Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_GAME_H_
#define GRIFFIN_GAME_H_

#include <memory>
#include <application/UpdateInfo.h>
#include <application/applicationTypedefs.h>

// Forward Declarations
class SDLApplication;

namespace griffin {

	void gameUpdateFrameTick(Game* pGame, Engine& engine, UpdateInfo& ui);

	void gameRenderFrameTick(Game* pGame, Engine& engine, float interpolation,
							 const int64_t realTime, const int64_t countsPassed);

	GamePtr make_game(const Engine& engine, const SDLApplication& app);
	void destroy_game(const GamePtr& gamePtr);

}

#endif