#pragma once
#ifndef GRIFFIN_GAME_SKYSYSTEM_H_
#define GRIFFIN_GAME_SKYSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <resource/ResourceLoader.h>

namespace griffin {
	using resource::ResourcePtr;
	using resource::CacheType;

	namespace game {

		struct SkySystem {
			ResourcePtr		skyBoxCubeMap = nullptr;	//<! space skybox cubemap texture

			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			void init(Game* pGame, const Engine& engine, const SDLApplication& app);
		};

	}
}

#endif