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
	auto timestamp = Timer::queryCounts();

	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			if (event.key.repeat == 0) {
				SDL_Log("key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu\n",
						event.type, event.key.state, event.key.keysym.scancode, event.key.repeat, timestamp);

				m_eventsQueue.push({ InputEventType::Keyboard_T, timestamp, std::move(event) });
			}
			handled = true;
			break;
		}

		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:

		case SDL_MOUSEMOTION: {
			/*SDL_Log("mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
					event.type, event.motion.which, event.motion.state, event.motion.windowID,
					event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);
					*/
			m_eventsQueue.push({ InputEventType::Mouse_T, timestamp, std::move(event) });
			handled = true;
			break;
		}
		case SDL_MOUSEWHEEL: {
			SDL_Log("mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.wheel.which, event.wheel.windowID,
					event.wheel.x, event.wheel.y, timestamp);

			m_eventsQueue.push({ InputEventType::Mouse_T, timestamp, std::move(event) });
			handled = true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			SDL_Log("mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.button.which, event.button.button, event.button.state,
					event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);

			m_eventsQueue.push({ InputEventType::Mouse_T, timestamp, std::move(event) });
			handled = true;
			break;
		}
		
		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
		case SDL_JOYAXISMOTION:
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION: {
			// SDL_JoystickGetAxis
			handled = true;
			break;
		}
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP: {
			SDL_Log("joystick button event=%d: which=%d: button=%d: state=%d: realTime=%lu\n",
					event.type, event.jbutton.which, event.jbutton.button, event.jbutton.state, timestamp);

			m_eventsQueue.push({ InputEventType::Joystick_T, timestamp, std::move(event) });
			handled = true;
			break;
		}

		case SDL_FINGERMOTION:
		case SDL_FINGERDOWN:
		case SDL_FINGERUP: {
			handled = true;
			break;
		}
	}
	return handled;
}


void InputSystem::initialize() // should this be the constructor?
{
	// the data structure that holds all of the metadata queried here should use the reflection
	// system, and be made available to Lua script

	// Initialize the mouse cursors table
	m_cursors[Cursor_Arrow_T]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	m_cursors[Cursor_Hand_T]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	m_cursors[Cursor_Wait_T]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	m_cursors[Cursor_IBeam_T]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	m_cursors[Cursor_Crosshair_T] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);


	// Initialize the joysticks
	int numJoysticks = SDL_NumJoysticks();
	for (int j = 0; j < numJoysticks; ++j) {
		// Open joystick
		SDL_Joystick* joy = SDL_JoystickOpen(j);

		if (joy) {
			m_joysticks.push_back(joy);

			SDL_Log("Opened Joystick %d", j);
			SDL_Log("  Name: %s", SDL_JoystickNameForIndex(j));
			SDL_Log("  Number of Axes: %d", SDL_JoystickNumAxes(joy));
			SDL_Log("  Number of Buttons: %d", SDL_JoystickNumButtons(joy));
			SDL_Log("  Number of Hats: %d", SDL_JoystickNumHats(joy));
			SDL_Log("  Number of Balls: %d", SDL_JoystickNumBalls(joy));
		} else {
			SDL_Log("Couldn't open Joystick %d", j);
		}
	}
	// The device_index passed as an argument refers to the N'th joystick presently recognized by SDL on the system.
	// It is NOT the same as the instance ID used to identify the joystick in future events.
	// See SDL_JoystickInstanceID() for more details about instance IDs.

	// Create the "global" input context, which eats all input, shows mouse cursor
	// replace this with loading of contexts from Lua
	{
		createContext(0);
	}
}

InputSystem::~InputSystem()
{
	// close all joysticks
	for (auto joy : m_joysticks) {
		if (SDL_JoystickGetAttached(joy)) {
			SDL_JoystickClose(joy);
		}
	}

	// free all cursors
	for (auto c : m_cursors) {
		SDL_FreeCursor(c);
	}

	// check memory reserves
	if (m_eventsQueue.unsafe_capacity() > RESERVE_INPUTSYSTEM_EVENTQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_EVENTQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_EVENTQUEUE, m_eventsQueue.unsafe_capacity());
	}
	if (m_popEvents.capacity() > RESERVE_INPUTSYSTEM_POPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_popEvents.capacity());
	}
}