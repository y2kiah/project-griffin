#include "../InputSystem.h"
#include <application/Timer.h>
#include <SDL_log.h>


using namespace griffin::core;

void InputSystem::update(const UpdateInfo& ui) {
	m_eventsQueue.try_pop_all_if(m_popEvents, [&](const InputEvent& i) {
		return (ui.virtualTime >= i.timeStampCounts);
	});

	for (const auto& e : m_popEvents) {
		SDL_Log("  Processed Input type=%d: realTime=%lu\n", e.evt.type, e.timeStampCounts);
	}
	m_popEvents.clear();
}


bool InputSystem::handleEvent(const SDL_Event& event) {
	bool handled = false;

	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			if (event.key.repeat == 0) {
				auto timestamp = Timer::queryCounts();
				SDL_Log("key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu\n",
						event.type, event.key.state, event.key.keysym.scancode, event.key.repeat, timestamp);

				m_eventsQueue.push({ timestamp, std::move(event) });
			}
			handled = true;
			break;
		}

		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:

		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:

		case SDL_JOYAXISMOTION:
		case SDL_JOYBALLMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYHATMOTION:

		case SDL_FINGERMOTION:
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:

		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED: {
			handled = true;
			break;
		}
	}
	return handled;
}