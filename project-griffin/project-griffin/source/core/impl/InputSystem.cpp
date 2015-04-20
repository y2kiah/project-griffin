#include "../InputSystem.h"
#include <application/Timer.h>
#include <SDL_log.h>

using namespace griffin;
using namespace griffin::core;


// class InputSystem

void InputSystem::update(const UpdateInfo& ui)
{
	// pop all events up to the game's virtual time
	m_eventsQueue.try_pop_all_if(m_popEvents, [&](const InputEvent& i) {
		return (ui.virtualTime >= i.timeStampCounts);
	});

	m_motionEventsQueue.try_pop_all(m_popMotionEvents); // need to pop all events because of the underlying use of vector_queue

	// map inputs using active context stack
	mapFrameInputs(ui);

	m_popEvents.clear();
}


void InputSystem::mapFrameInputs(const UpdateInfo& ui)
{
	// accumulate relative mouse movement for this frame
	int mouse_xrel = 0;
	int mouse_yrel = 0;

	// process all motion events, these go to axis mappings
	auto frameSize = m_popMotionEvents.size();
	for (int e = 0; e < frameSize; ++e) {
		const auto& motionEvt = m_popMotionEvents[e];

		// if timestamp is greater than frame time, we're done
		if (motionEvt.timeStampCounts > ui.virtualTime) {
			frameSize = e;
			break;
		}

		if (motionEvt.evt.type == SDL_MOUSEMOTION) {
			mouse_xrel += motionEvt.evt.motion.xrel;
			mouse_yrel += motionEvt.evt.motion.yrel;
		}
	}
	// erase up to the last event for this frame
	m_popMotionEvents.erase(m_popMotionEvents.begin(), m_popMotionEvents.begin() + frameSize);


	// process all other input events
	m_frameMappedInput.mappedInputs.clear();
	for (const auto& ac : m_activeInputContexts) {
		m_inputContexts[ac.contextId].getInputEventMappings(m_popEvents, m_frameMappedInput.mappedInputs);
	}

	// 
	//for (const auto& e : m_popEvents) {
	//	SDL_Log("  Processed Input type=%d: realTime=%lu\n", e.evt.type, e.timeStampCounts);
	//}
}


bool InputSystem::handleEvent(const SDL_Event& event)
{
	bool handled = false;
	auto timestamp = Timer::queryCounts();

	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			if (event.key.repeat == 0) {
				SDL_Log("key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu\n",
						event.type, event.key.state, event.key.keysym.scancode, event.key.repeat, timestamp);

				m_eventsQueue.push({ std::move(event), timestamp });
			}
			handled = true;
			break;
		}

		case SDL_TEXTEDITING: {
			SDL_Log("key event=%d: text=%s: length=%d: start=%d: windowID=%d: realTime=%lu\n",
					event.type, event.edit.text, event.edit.length, event.edit.start, event.edit.windowID, timestamp);
			
			m_eventsQueue.push({ std::move(event), timestamp });
			
			handled = true;
			break;
		}
		case SDL_TEXTINPUT: {
			SDL_Log("key event=%d: text=%s: windowID=%d: realTime=%lu\n",
					event.type, event.text.text, event.text.windowID, timestamp);
			
			m_eventsQueue.push({ std::move(event), timestamp });
			
			handled = true;
			break;
		}

		case SDL_MOUSEMOTION: {
			/*SDL_Log("mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
					event.type, event.motion.which, event.motion.state, event.motion.windowID,
					event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);
					*/
			m_motionEventsQueue.push({ std::move(event), timestamp });
			handled = true;
			break;
		}
		case SDL_MOUSEWHEEL: {
			SDL_Log("mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.wheel.which, event.wheel.windowID,
					event.wheel.x, event.wheel.y, timestamp);
			
			m_eventsQueue.push({ std::move(event), timestamp });
			handled = true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			SDL_Log("mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.button.which, event.button.button, event.button.state,
					event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);
			
			m_eventsQueue.push({ std::move(event), timestamp });
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
			
			m_eventsQueue.push({ std::move(event), timestamp });
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

}

Id_T InputSystem::createContext(uint16_t optionsMask, uint8_t priority, bool makeActive)
{
	//auto f = tss_([optionsMask](ThreadSafeState& tss_) {
	//	return tss_.m_inputContexts.emplace(optionsMask);
	//});
	//return f;

	auto contextId = m_inputContexts.emplace(optionsMask);
	m_activeInputContexts.push_back({ contextId, priority, makeActive });
	std::stable_sort(m_activeInputContexts.begin(), m_activeInputContexts.end(),
					[](const ActiveInputContext& i, const ActiveInputContext& j) {
						return i.priority < j.priority;
					});

	return contextId;
}

bool InputSystem::makeContextActive(Id_T contextId)
{
	if (m_inputContexts.isValid(contextId)) {
		for (auto& ac : m_activeInputContexts) {
			if (ac.contextId == contextId) {
				ac.active = true;
				break;
			}
		}
		return true;
	}
	return false;
}

void InputSystem::startTextInput()
{
	SDL_StartTextInput();
}

void InputSystem::stopTextInput()
{
	SDL_StopTextInput();
}

bool InputSystem::isTextInputActive() const
{
	return (SDL_IsTextInputActive() == SDL_TRUE);
}

void InputSystem::startRelativeMouseMode()
{
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void InputSystem::stopRelativeMouseMode()
{
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

bool InputSystem::isRelativeMouseModeActive() const
{
	return (SDL_GetRelativeMouseMode() == SDL_TRUE);
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
	if (m_eventsQueue.unsafe_capacity() > RESERVE_INPUTSYSTEM_EVENTSQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_EVENTSQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_EVENTSQUEUE, m_eventsQueue.unsafe_capacity());
	}
	if (m_motionEventsQueue.unsafe_capacity() > RESERVE_INPUTSYSTEM_MOTIONEVENTSQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_MOTIONEVENTSQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_MOTIONEVENTSQUEUE, m_motionEventsQueue.unsafe_capacity());
	}
	if (m_popEvents.capacity() > RESERVE_INPUTSYSTEM_POPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_popEvents.capacity());
	}
	if (m_popMotionEvents.capacity() > RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE, m_popMotionEvents.capacity());
	}
	if (m_frameMappedInput.mappedInputs.capacity() > RESERVE_INPUTSYSTEM_POPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_frameMappedInput.mappedInputs.capacity());
	}
}


// class InputContext

void InputContext::getInputEventMappings(vector<InputEvent>& inputEvents, vector<MappedInput>& mappedInputs)
{
	for (auto& evt : inputEvents) {
		for (const auto& mapping : m_inputMappings) {
			//evt.evt.type
		}
	}
}