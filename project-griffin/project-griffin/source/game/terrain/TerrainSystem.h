#pragma once
#ifndef GRIFFIN_GAME_TERRAINSYSTEM_H_
#define GRIFFIN_GAME_TERRAINSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <resource/ResourceLoader.h>

namespace griffin {
	using resource::ResourcePtr;
	using resource::CacheType;

	namespace game {

		struct TerrainSystem {
			// Variables
			
			
			ResourcePtr terrainProgram = nullptr;		//<! terrain shader program

			// Public Functions

			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			
			void renderFrameTick(Game* pGame, Engine& engine, float interpolation,
								 const int64_t realTime, const int64_t countsPassed);

			void init(Game* pGame, const Engine& engine, const SDLApplication& app);
		};

	}
}

#endif