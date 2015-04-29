#include "../InputSystem.h"
#include <application/Timer.h>
#include <SDL_log.h>

using namespace griffin;
using namespace griffin::core;


// class InputSystem

void InputSystem::update(const UpdateInfo& ui)
{
	// pop all events up to the game's virtual time, key events are kept in buffer for the frame
	m_popEvents.clear();
	m_eventsQueue.try_pop_all_if(m_popEvents, [&](const InputEvent& i) {
		return (ui.virtualTime >= i.timeStampCounts);
	});

	m_motionEventsQueue.try_pop_all(m_popMotionEvents); // need to pop all events because of the underlying use of vector_queue

	// map inputs using active context stack
	mapFrameInputs(ui);
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

		switch (motionEvt.evt.type) {
			case SDL_MOUSEMOTION: {
				mouse_xrel += motionEvt.evt.motion.xrel;
				mouse_yrel += motionEvt.evt.motion.yrel;
				break;
			}
			case SDL_JOYAXISMOTION: {

				break;
			}
		}

	}
	// erase up to the last event for this frame
	m_popMotionEvents.erase(m_popMotionEvents.begin(), m_popMotionEvents.begin() + frameSize);

	// clear previous frame actions
	m_frameMappedInput.actions.clear();
	// TODO: loop over previous frame states, remove states that are not mappings in any active context
	// also add to totalCounts, totalMS and totalFrames

	// process all other input events
	for (auto& evt : m_popEvents) {							// for each input event
		bool matched = false;

		for (const auto& ac : m_activeInputContexts) {		// for each active context
			auto& context = m_inputContexts[ac.contextId];
			
			// handle key and button events
			if (evt.eventType == Event_Keyboard_T ||
				evt.eventType == Event_Joystick_T ||
				evt.eventType == Event_Mouse_T)
			{
				for (Id_T mappingId : context.inputMappings) {	// check all input mappings for a match
					const auto& mapping = m_inputMappings[mappingId];

					// check for action mappings
					if (mapping.type == Action_T) {
						MappedAction ma;

						if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_KEYDOWN) ||
							(mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_KEYUP))
						{
							matched = (evt.evt.key.keysym.sym == mapping.button &&
									   evt.evt.key.keysym.mod == mapping.modifier);
						}
						else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
								 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
						{
							matched = (evt.evt.button.button == mapping.button &&
									   evt.evt.button.clicks == mapping.clicks);
							if (matched) {
								ma.xRaw = evt.evt.button.x;
								ma.yRaw = evt.evt.button.y;
							}
						}
						else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
								 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
						{
							matched = (evt.evt.jbutton.which == mapping.instanceId &&
									   evt.evt.jbutton.button == mapping.button);
						}
						else if (evt.evt.type == SDL_MOUSEWHEEL)
						{
							// add a way to map a mouse wheel binding
							/*evt.evt.wheel.x 
							if (matched) {
								mi.xRaw = evt.evt.button.x;
								mi.yRaw = evt.evt.button.y;
							}*/
						}

						// found a matching action mapping for the event
						if (matched) {
							ma.mappingId = mappingId;
							ma.inputMapping = &mapping;
							
							m_frameMappedInput.actions.push_back(std::move(ma));
							break; // skip checking the rest of the mappings
						}
					}

					// check for state mappings
					else if (mapping.type == State_T) {
						auto stateIndex = findActiveState(mappingId);
						bool stateActive = (stateIndex != -1);

						if (!stateActive) {
							if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_KEYDOWN) ||
								(mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_KEYUP))
							{
								matched = (evt.evt.key.keysym.sym == mapping.button &&
										   evt.evt.key.keysym.mod == mapping.modifier);
							}
							else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == mapping.button);
							}
							else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = (evt.evt.jbutton.which == mapping.instanceId &&
										   evt.evt.jbutton.button == mapping.button);
							}
						}
						else { // stateActive
							if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_KEYDOWN) ||
								(mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_KEYUP))
							{
								matched = (evt.evt.key.keysym.sym == mapping.button &&
										   evt.evt.key.keysym.mod == mapping.modifier);
								// TODO: maybe need to look for sym and mod keys separately here and make state inactive for either one
							}
							else if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == mapping.button);
							}
							else if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = (evt.evt.jbutton.which == mapping.instanceId &&
										   evt.evt.jbutton.button == mapping.button);
							}
						}

						// found a new active state mapping with this event
						if (matched && !stateActive) {
							MappedState ms;
							ms.mappingId = mappingId;
							ms.inputMapping = &mapping;
							ms.startCounts = ui.gameTime;
							ms.startFrame = ui.frame;

							m_frameMappedInput.states.push_back(std::move(ms));
							break; // skip checking the rest of the mappings
						}
						// make state inactive
						else if (matched && stateActive) {
							// make inactive, remove from states list by swap with last and pop_back
							if (m_frameMappedInput.states.size() > 1) {
								std::swap(m_frameMappedInput.states.at(stateIndex), m_frameMappedInput.states.back());
							}
							m_frameMappedInput.states.pop_back();
							break;
						}
					}
				}

				// found a mapping, move on to the next input event
				if (matched) { break; }
			}
			// handle text input events
			else if (context.options[CaptureTextInput_T] && evt.eventType == Event_TextInput_T) {
				if (evt.evt.type == SDL_TEXTINPUT) {
					//m_frameMappedInput.textInput += std::wstring(evt.evt.text.text);
				}
				break;
			}

			// didn't find a match, check if this context eats input events or passes them down
			if (context.options[EatMouseEvents_T]    && evt.eventType == Event_Mouse_T)    { break; }
			if (context.options[EatJoystickEvents_T] && evt.eventType == Event_Joystick_T) { break; }
			if (context.options[EatKeyboardEvents_T] &&
				(evt.eventType == Event_Keyboard_T || evt.eventType == Event_TextInput_T)) { break; }
		}
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

				m_eventsQueue.push({ Event_Keyboard_T, std::move(event), timestamp });
			}
			handled = true;
			break;
		}

		case SDL_TEXTEDITING: {
			SDL_Log("key event=%d: text=%s: length=%d: start=%d: windowID=%d: realTime=%lu\n",
					event.type, event.edit.text, event.edit.length, event.edit.start, event.edit.windowID, timestamp);

			m_eventsQueue.push({ Event_TextInput_T, std::move(event), timestamp });

			handled = true;
			break;
		}
		case SDL_TEXTINPUT: {
			SDL_Log("key event=%d: text=%s: windowID=%d: realTime=%lu\n",
					event.type, event.text.text, event.text.windowID, timestamp);

			m_eventsQueue.push({ Event_TextInput_T, std::move(event), timestamp });

			handled = true;
			break;
		}

		case SDL_MOUSEMOTION:
			/*SDL_Log("mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
					event.type, event.motion.which, event.motion.state, event.motion.windowID,
					event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);*/
		case SDL_JOYAXISMOTION:
			/*SDL_Log("joystick motion event=%d: which=%d: axis=%d: value=%d: realTime=%lu\n",
					event.type, event.jaxis.which, event.jaxis.axis, event.jaxis.value, timestamp);*/
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION: {
			m_motionEventsQueue.push({ Event_Mouse_T, std::move(event), timestamp });
			handled = true;
			break;
		}

		case SDL_MOUSEWHEEL: {
			SDL_Log("mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.wheel.which, event.wheel.windowID,
					event.wheel.x, event.wheel.y, timestamp);

			m_eventsQueue.push({ Event_Mouse_T, std::move(event), timestamp });
			handled = true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			SDL_Log("mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.button.which, event.button.button, event.button.state,
					event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);

			m_eventsQueue.push({ Event_Mouse_T, std::move(event), timestamp });
			handled = true;
			break;
		}

		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED: {

			handled = true;
			break;
		}
		
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP: {
			SDL_Log("joystick button event=%d: which=%d: button=%d: state=%d: realTime=%lu\n",
					event.type, event.jbutton.which, event.jbutton.button, event.jbutton.state, timestamp);
			
			m_eventsQueue.push({ Event_Joystick_T, std::move(event), timestamp });
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


size_t InputSystem::findActiveState(Id_T mappingId) const
{
	for (auto s = m_frameMappedInput.states.size() - 1; s >= 0; s--) {
		if (m_frameMappedInput.states[s].mappingId.value == mappingId.value) { return s; }
	}
	return -1;
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
	m_joysticks.reserve(numJoysticks);
	m_frameMappedInput.joystickMotion.reserve(numJoysticks);

	for (int j = 0; j < numJoysticks; ++j) {
		// Open joystick
		SDL_Joystick* joy = SDL_JoystickOpen(j);

		if (joy != nullptr) {
			m_joysticks.push_back(joy);
			m_frameMappedInput.joystickMotion.emplace_back();

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

bool InputSystem::textInputActive() const
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

bool InputSystem::relativeMouseModeActive() const
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
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE (popEvents): original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_popEvents.capacity());
	}
	if (m_popMotionEvents.capacity() > RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE: original=%d, highest=%d", RESERVE_INPUTSYSTEM_MOTIONPOPQUEUE, m_popMotionEvents.capacity());
	}
	if (m_inputMappings.capacity() > RESERVE_INPUTSYSTEM_MAPPINGS) {
		SDL_Log("check RESERVE_INPUTSYSTEM_MAPPINGS: original=%d, highest=%d", RESERVE_INPUTSYSTEM_MAPPINGS, m_inputMappings.capacity());
	}
	if (m_inputContexts.capacity() > RESERVE_INPUTSYSTEM_CONTEXTS) {
		SDL_Log("check RESERVE_INPUTSYSTEM_CONTEXTS: original=%d, highest=%d", RESERVE_INPUTSYSTEM_CONTEXTS, m_inputContexts.capacity());
	}
	// FrameMappedInput reserves
	if (m_frameMappedInput.actions.capacity() > RESERVE_INPUTSYSTEM_POPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE (actions): original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_frameMappedInput.actions.capacity());
	}
	if (m_frameMappedInput.states.capacity() > RESERVE_INPUTSYSTEM_POPQUEUE) {
		SDL_Log("check RESERVE_INPUTSYSTEM_POPQUEUE (states): original=%d, highest=%d", RESERVE_INPUTSYSTEM_POPQUEUE, m_frameMappedInput.states.capacity());
	}
}


// class InputContext

InputContext::~InputContext()
{
	if (inputMappings.capacity() > RESERVE_INPUTCONTEXT_MAPPINGS) {
		SDL_Log("check RESERVE_INPUTCONTEXT_MAPPINGS: original=%d, highest=%d", RESERVE_INPUTCONTEXT_MAPPINGS, inputMappings.capacity());
	}
}