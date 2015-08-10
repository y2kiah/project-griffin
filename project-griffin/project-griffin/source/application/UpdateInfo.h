#pragma once
#ifndef GRIFFIN_UPDATEINFO_H_
#define GRIFFIN_UPDATEINFO_H_

#include <cstdint>

namespace griffin {

	struct UpdateInfo {
		int64_t		virtualTime;	//<! interpolation of real time with each step of update loop, keeps in sync with real time, useful for checking against timestamped events 
		int64_t		gameTime;		//<! current game time from 0, useful for logical game time checks, sensitive to gameSpeed, should be reset to 0 when game is restarted
		int64_t		deltaCounts;	//<! update frame clock counts
		uint64_t	frame;			//<! frame counter, starts at 0
		float		deltaMs;		//<! update frame time in milliseconds
		float		deltaT;			//<! update frame time in seconds
		float		gameSpeed;		//<! rate multiplier of gameplay, 1.0 is normal
	};

}
#endif