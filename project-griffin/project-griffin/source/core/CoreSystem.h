#pragma once

#include <cstdint>

namespace griffin {
	namespace core {


		// use sean parent's internal polymorphic implementation concept/model
		class CoreSystem {
		public:
			virtual ~CoreSystem() = 0 {};

			struct UpdateInfo {
				int64_t	virtualTime;
				int64_t	gameTime;
				int64_t	deltaCounts;
				double	deltaMs;
				double	gameSpeed;
				int		frame;
			};

			virtual void update(const UpdateInfo& ui) = 0;
		};

	}
}