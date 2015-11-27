#pragma once
#ifndef GRIFFIN_GAME_DEVCONSOLESYSTEM_H_
#define GRIFFIN_GAME_DEVCONSOLESYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>


namespace griffin {
	namespace game {

		struct DevConsoleSystem {
			bool		visible = false;
			uint32_t	toggleVisible = 0;

			Id_T		devCameraInputContextId;				//<! handle for the devconsole input context
			Id_T		toggleId;								//<! input to turn devconsole on/off

			void updateFrameTick(Game* pGame, Engine& engine, const UpdateInfo& ui);
			void init(Game* pGame, const Engine& engine, const SDLApplication& app);

		};
	}
}

#endif