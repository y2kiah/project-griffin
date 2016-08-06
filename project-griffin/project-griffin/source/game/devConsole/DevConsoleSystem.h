#pragma once
#ifndef GRIFFIN_GAME_DEVCONSOLESYSTEM_H_
#define GRIFFIN_GAME_DEVCONSOLESYSTEM_H_

#include <game/Game.h>
#include <application/Engine.h>
#include <application/UpdateInfo.h>


namespace griffin {
	namespace game {

		struct DevConsoleSystem {
			bool		visible = false;				//<! whether console is shown or not
			bool		wasRelativeMouseMode = false;	//<! true if relative mouse mode was active when console shown
			uint32_t	toggleVisible = 0;

			Id_T		devConsoleInputContextId;		//<! handle for the devconsole input context
			Id_T		toggleId;						//<! input to show/hide devconsole

			void renderFrameTick(Game& game, Engine& engine, float interpolation,
								 const int64_t realTime, const int64_t countsPassed);

			void init(Game& game, const Engine& engine, const SDLApplication& app);

		};
	}
}

#endif