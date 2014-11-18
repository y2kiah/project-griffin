#pragma once

#include <core/CoreSystem.h>
#include <vector>
#include <utility/container/concurrent_queue.h>
#include <SDL_events.h>

using std::vector;

namespace griffin {
	namespace core {

		struct InputEvent {
			int64_t		timeStampCounts;
			SDL_Event	evt;
		};


		class InputSystem : public CoreSystem {
		public:
			~InputSystem() {}

			/**
			 * Executed on the update/render thread
			 */
			void update(const UpdateInfo& ui);

			/**
			* Executed on the input/GUI thread
			*/
			bool handleEvent(const SDL_Event& event);

		private:
			concurrent_queue<InputEvent>	m_eventsQueue;	//<! push on input thread, pop on update thread
			vector<InputEvent>				m_popEvents;	//<! pop events from the queue into this buffer
		};


		class InputDispatcher {
		public:

		private:

		};

	}
}