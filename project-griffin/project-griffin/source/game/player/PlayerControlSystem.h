#pragma once
#ifndef GRIFFIN_GAME_PLAYERCONTROLSYSTEM_H_
#define GRIFFIN_GAME_PLAYERCONTROLSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <glm/vec3.hpp>
#include <entity/EntityTypedefs.h>


namespace griffin {
	namespace game {
		using entity::EntityId;
		using entity::ComponentId;


		struct PlayerControlSystem {
			enum SpeedFlag : int {
				SpeedFlag_Normal = 0,
				SpeedFlag_Sprint = 1,
				SpeedFlag_Walk   = 2
			};

			int			moveForward = 0, moveSide = 0;
			int			pitchRaw = 0,    yawRaw = 0;
			float		pitchMapped = 0, yawMapped = 0;
			int			speedToggle = SpeedFlag_Normal;			//<! 0=normal, 1=sprint, 2=walk
			glm::vec3	velocity = {};							//<! current player velocity
			
			bool		crouching = false;
			float		crouchT = 0.0f;							//<! crouch interpolation [0-1]
			float		headBob = 0;							//<! current head bob value passed into sin(t)

			EntityId	playerId;								//<! entity id of the player
			ComponentId	movementComponentId;					//<! movement component for the player entity
			Id_T		playerfpsInputContextId;				//<! handle for the playerfps input context
			Id_T		forwardId, backId, leftId, rightId;		//<! mapping handles for the inputs
			Id_T		sprintId, walkId, crouchId;
			Id_T		lookXId, lookYId;

			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			void init(Game& game, const Engine& engine, const SDLApplication& app);

		};
	}
}

#endif