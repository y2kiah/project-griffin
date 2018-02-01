#pragma once
#ifndef GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_
#define GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <entity/EntityTypedefs.h>


namespace griffin {
	namespace game {
		using entity::EntityId;
		using entity::ComponentId;
		using scene::SceneNodeId;


		struct ScreenShakerSystem {

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			
			void renderFrameTick(Game& game, Engine& engine, float interpolation,
								 const int64_t realTime, const int64_t countsPassed);

			void init(Game& game, const Engine& engine, const SDLApplication& app);
			

			/**
			* Adds a new ScreenShakeProducer component to an existing entity.
			* @param turbulence  starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
			* @param totalTimeToLiveMS  total time the shaker is active, turbulence goes from max to 0 linearly by time
			* @param radius  radius of the effective area in feet
			* @return  id of the new ScreenShakeProducer component
			*/
			ComponentId addScreenShakeProducerToEntity(
							EntityId entityId,
							SceneNodeId sceneNodeId,
							float turbulence,
							float totalTimeToLiveMS,
							float radius);
			
			/**
			* Some screen shake effects are not centered at a point in 3d space, and simply need
			* to be added to the player so that they always take effect regardless of position.
			* @param turbulence  starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
			* @param totalTimeToLiveMS  total time the shaker is active, turbulence goes from max to 0 linearly by time
			* @return  id of the new ScreenShakeProducer component
			*/
			ComponentId addScreenShakeProducerToPlayer(
							EntityId playerId,
							float turbulence,
							float totalTimeToLiveMS = 0.0f);


		};
	}
}

#endif