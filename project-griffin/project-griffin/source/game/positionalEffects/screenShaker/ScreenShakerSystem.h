#pragma once
#ifndef GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_
#define GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <entity/EntityTypedefs.h>
//#include <glm/vec3.hpp>


namespace griffin {
	namespace game {
		using entity::EntityId;
		using entity::ComponentId;


		struct ScreenShakerComponent {
			float	turbulence;		//<! strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
			float	radiusSq;		//<! radius squared of the effective area
		};

		struct ScreenShakerSystem {
			float		frameTotalPitchDelta = 0.0f;			//<! angles aggregated from active screen shakers
			float		frameTotalYawDelta = 0.0f;
			float		frameTotalRollDelta = 0.0f;

			EntityId	playerId;								//<! entity id of the player
			ComponentId	movementComponentId;					//<! movement component for the player entity

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			void init(Game& game, const Engine& engine, const SDLApplication& app);

			ComponentId addScreenShakerToEntity(EntityId entityId);//, ...);
			ComponentId addScreenShakerToPlayer();


		};
	}
}

#endif