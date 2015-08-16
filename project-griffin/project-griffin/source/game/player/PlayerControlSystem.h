#pragma once
#ifndef GRIFFIN_GAME_PLAYERCONTROLSYSTEM_H_
#define GRIFFIN_GAME_PLAYERCONTROLSYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>
#include <glm/vec3.hpp>


namespace griffin {
	namespace game {

		struct PlayerControlSystem {
			enum SpeedFlag : int {
				SpeedFlag_Normal = 0,
				SpeedFlag_Sprint = 1,
				SpeedFlag_Walk   = 2
			};

			int			moveForward, moveSide;
			int			pitchRaw, yawRaw;
			float		pitchMapped, yawMapped;
			int			speedToggle = SpeedFlag_Normal;			//<! 0=normal, 1=sprint, 2=walk
			glm::vec3	velocity = {};							//<! current player velocity

			Id_T		playerId;								//<! entity id of the player
			Id_T		movementComponentId;					//<! movement component for the player entity
			Id_T		playerfpsInputContextId;				//<! handle for the playerfps input context
			Id_T		forwardId, backId, leftId, rightId;		//<! mapping handles for the inputs
			Id_T		sprintId, walkId, lookXId, lookYId;

			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			void init(Game* pGame, const Engine& engine, const SDLApplication& app);

		};
	}
}

#endif