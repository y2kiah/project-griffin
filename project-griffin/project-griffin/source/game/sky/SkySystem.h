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
			ResourcePtr	skyBoxCubeMap = nullptr;		//<! space skybox cubemap texture
			
			ResourcePtr transmittanceProgram = nullptr;	//<! atmospheric scattering pre-computation shaders
			ResourcePtr irradiance1Program = nullptr;
			ResourcePtr irradianceNProgram = nullptr;
			ResourcePtr inscatter1Program = nullptr;
			ResourcePtr	inscatterNProgram = nullptr;
			ResourcePtr	inscatterSProgram = nullptr;
			ResourcePtr	copyIrradianceProgram = nullptr;
			ResourcePtr	copyInscatter1Program = nullptr;
			ResourcePtr	copyInscatterNProgram = nullptr;
			
			ResourcePtr	atmosphereProgram = nullptr;	//<! post-process atmospheric scattering shader

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			void init(Game& game, const Engine& engine, const SDLApplication& app);
		};

	}
}

#endif