#pragma once
#ifndef GRIFFIN_UPDATEINFO_H_
#define GRIFFIN_UPDATEINFO_H_

#include <cstdint>

namespace griffin {

	struct UpdateInfo {
		int64_t		virtualTime;
		int64_t		gameTime;
		int64_t		deltaCounts;
		float		deltaMs;
		float		gameSpeed;
		uint64_t	frame;
	};

}
#endif