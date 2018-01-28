#pragma once
#ifndef GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_
#define GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
//#include <glm/vec3.hpp>


namespace griffin {
	namespace game {

		struct ScreenShakerSystem {
			float		frameTotalPitchDelta = 0.0f;			//<! angles aggregated from active screen shakers
			float		frameTotalYawDelta = 0.0f;
			float		frameTotalRollDelta = 0.0f;

			Id_T		playerId;								//<! entity id of the player
			Id_T		movementComponentId;					//<! movement component for the player entity

			// For testing
			Id_T		playerfpsInputContextId;				//<! handle for the playerfps input context

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			void init(Game& game, const Engine& engine, const SDLApplication& app,
			          Id_T _playerId, Id_T _playerfpsInputContextId);

		};
	}
}

#endif