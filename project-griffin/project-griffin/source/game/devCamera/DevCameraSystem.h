#pragma once
#ifndef GRIFFIN_GAME_DEVCAMERASYSTEM_H_
#define GRIFFIN_GAME_DEVCAMERASYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>


namespace griffin {
	namespace game {

		struct DevCameraSystem {
			// TEMP, not really needed, just a demonstration of adding a script component
			/*struct DevCameraMovementComponent {
				int		moveForward;
				int		moveSide;
				int		moveVertical;
				bool	speedToggle;
			};*/

			int			moveForward = 0, moveSide = 0, moveVertical = 0, roll = 0;
			int			pitchRaw = 0, yawRaw = 0;
			float		pitchMapped = 0, yawMapped = 0;
			float		speed = 10.0f;							//<! in ft/s
			bool		speedToggle = false;
			bool		active = false;
			uint32_t	toggleActiveCamera = 0;

			Id_T		devCameraId;							//<! entity id of the dev camera
			Id_T		movementComponentId;					//<! movement component for the dev camera entity

			Id_T		devCameraInputContextId;				//<! handle for the devcamera input context
			Id_T		playerfpsInputContextId;				//<! handle for the playerfps input context
			Id_T		toggleId;								//<! input to turn devcamera on/off
			Id_T		forwardId, backId, leftId, rightId;		//<! mapping handles for the inputs
			Id_T		rollLeftId, rollRightId, upId, downId;
			Id_T		speedToggleId, speedScrollIncreaseId, speedScrollDecreaseId;
			Id_T		lookXId, lookYId;


			void updateFrameTick(Game& game, Engine& engine, const UpdateInfo& ui);
			void init(Game& game, const Engine& engine, const SDLApplication& app);
			
		};
	}
}

#endif