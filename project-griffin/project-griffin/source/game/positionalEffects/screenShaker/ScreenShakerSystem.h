#pragma once
#ifndef GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_
#define GRIFFIN_GAME_SCREENSHAKERSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <entity/components.h>
//#include <glm/vec3.hpp>


namespace griffin {
	namespace game {
		using entity::EntityId;
		using entity::ComponentId;

		struct ScreenShakerSystem {
			float		frameTotalPitchDelta = 0.0f;			//<! angles aggregated from active screen shakers
			float		frameTotalYawDelta = 0.0f;
			float		frameTotalRollDelta = 0.0f;

			EntityId	playerId;								//<! entity id of the player

			
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
			ComponentId addScreenShakeProducerToEntity(EntityId entityId, float turbulence, float totalTimeToLiveMS, float radius);
			
			/**
			* Some screen shake effects are not centered at a point in 3d space, and simply need
			* to be added to the player so that they always take effect regardless of position.
			* @param turbulence  starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe
			* @param totalTimeToLiveMS  total time the shaker is active, turbulence goes from max to 0 linearly by time
			* @return  id of the new ScreenShakeProducer component
			*/
			ComponentId addScreenShakeProducerToPlayer(float turbulence, float totalTimeToLiveMS = 0.0f);


		};


		/**
		* Causes shake on nearby ScreenShakeNodes
		*/
		COMPONENT(ScreenShakeProducer,
			(float, startTurbulence,,	"starting strength of screen shake, 0=none, 0.25=light, 0.75=heavy, 1.0=severe"),
			(float, turbulence,,		"current turbulence level"),
			(float, totalTimeToLiveMS,,	"total time the shaker is active, turbulence goes from startTurbulence to 0 linearly over this time"),
			(float, radius,,			"radius of the effective area in feet")
		)

		/**
		* Pairs with a parent SceneNode and receives shake from nearby ScreenShakeProducers.
		*/
		COMPONENT(ScreenShakeNode,
			(ComponentId, sceneNodeId,,	"SceneNode of CameraInstance to base shake angles on."),
			(float, prevTurbulence,,	"previous effective turbulence used for interpolation"),
			(float, nextTurbulence,,	"next effective turbulence used for interpolation")
		)
	}
}

#endif