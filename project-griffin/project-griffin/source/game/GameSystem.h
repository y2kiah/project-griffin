#pragma once
#ifndef GRIFFIN_GAMESYSTEM_H_
#define GRIFFIN_GAMESYSTEM_H_


#include <application/UpdateInfo.h>


namespace griffin {
	namespace game {


		// use sean parent's internal polymorphic implementation concept/model?
		class GameSystem {
		public:
			virtual ~GameSystem() = 0 {}

			virtual void updateFrameTick(const UpdateInfo& ui) = 0;
			virtual void renderFrameTick(float interpolation) = 0;
		};

	}
}

#endif