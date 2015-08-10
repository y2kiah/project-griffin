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
			struct DevCameraMovementComponent {
				int		moveForward;
				int		moveSide;
				int		moveVertical;
				bool	speedToggle;
			};

			int		moveForward, moveSide, moveVertical, roll;
			int		pitchRaw, yawRaw;
			float	pitchMapped, yawMapped;
			bool	speedToggle;

			Id_T	devCameraId;							//<! entity id of the dev camera
			Id_T	devCameraMovementId;					//<! dev camera movement component id

			Id_T	devCameraInputContextId;				//<! input context handle for the devcamera
			Id_T	toggleId;								//<! input to turn devcamera on/off
			Id_T	forwardId, backId, leftId, rightId;		//<! mapping handles for the inputs
			Id_T	rollLeftId, rollRightId, upId, downId;
			Id_T	speedToggleId, lookXId, lookYId;


			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			void init(Game* pGame, const Engine& engine, const SDLApplication& app);
			
		};
	}
}

#endif