#include <input/InputSystem.h>
#include <application/main.h>
#include <application/Timer.h>
#include <SDL_log.h>

using namespace griffin;
using namespace griffin::input;

#define JOYSTICK_INVERSE_MAX_RAW	1.0f / 32768.0f


// class InputSystem

const SDLApplication* InputSystem::app = nullptr;


void InputSystem::updateFrameTick(const UpdateInfo& ui)
{
	// pop all events up to the game's virtual time, key events are kept in buffer for the frame
	m_popEvents.clear();
	m_eventsQueue.try_pop_all_if(m_popEvents, [&](const InputEvent& i) {
		return (ui.virtualTime >= i.timeStampCounts);
	});

	m_motionEventsQueue.try_pop_all(m_popMotionEvents); // need to pop all motion events because of the underlying use of vector_queue

	// clear previous frame actions and axis mappings
	m_frameMappedInput.actions.clear();
	m_frameMappedInput.axes.clear();

	// remove active states that are no longer mappings in any active context
	std::remove_if(m_frameMappedInput.states.begin(), m_frameMappedInput.states.end(),
				   [&](const MappedState& state) {
		for (const auto& ac : m_activeInputContexts) {
			if (!ac.active) { continue; }
			auto& context = m_inputContexts[ac.contextId];
			if (std::find(context.inputMappings.begin(), context.inputMappings.end(), state.mappingId) != context.inputMappings.end()) {
				return false;
			}
		}
		return true;
	});

	// loop over active states, add to totalCounts, totalMS and totalFrames, reset handled flag
	for (auto& state : m_frameMappedInput.states) {
		state.totalFrames += 1;
		state.totalCounts += ui.deltaCounts;
		state.totalMs += ui.deltaMs;
		state.handled = false;
	}

	// map inputs using active contexts
	mapFrameInputs(ui);
	mapFrameMotion(ui);

	// TEMP output mapped inputs
	for (const auto& s : m_frameMappedInput.states) {
		SDL_Log("state \"%s\" active", s.inputMapping->name);
	}
	for (const auto& a : m_frameMappedInput.actions) {
		SDL_Log("action \"%s\" triggered", a.inputMapping->name);
	}

	// invoke all callbacks, passing this frame's mapped input
	// TODO: needs to be done in priority order
	for (auto& cb : m_callbacks.getItems()) {
		cb(m_frameMappedInput);
	}
}


void InputSystem::mapFrameInputs(const UpdateInfo& ui)
{
	// process this frame's input events
	for (auto& evt : m_popEvents) {							// for each input event
		bool matched = false;

		for (const auto& ac : m_activeInputContexts) {		// for each active context
			// skip contexts that aren't active
			if (!ac.active) { continue; }

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
						MappedAction ma{};

						if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_KEYDOWN) ||
							(mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_KEYUP))
						{
							matched = (evt.evt.key.keysym.sym == mapping.keycode &&
									   evt.evt.key.keysym.mod == mapping.modifier);
						}
						else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
								 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
						{
							matched = (evt.evt.button.button == mapping.keycode &&
									   evt.evt.button.clicks == mapping.clicks);
							if (matched) {
								ma.xRaw = evt.evt.button.x;
								ma.yRaw = evt.evt.button.y;
							}
						}
						else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
								 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
						{
							matched = (evt.evt.jbutton.which == mapping.device &&
									   evt.evt.jbutton.button == mapping.keycode);
						}
						else if (evt.evt.type == SDL_MOUSEWHEEL)
						{
							matched = (mapping.mouseWheel == 1 &&
									   (mapping.axis == 0 && mapping.bindIn == Bind_Up_T   && evt.evt.wheel.x > 0) ||
									   (mapping.axis == 0 && mapping.bindIn == Bind_Down_T && evt.evt.wheel.x < 0) ||
									   (mapping.axis == 1 && mapping.bindIn == Bind_Up_T   && evt.evt.wheel.y > 0) ||
									   (mapping.axis == 1 && mapping.bindIn == Bind_Down_T && evt.evt.wheel.y < 0));
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
								(mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_KEYUP) &&
								(mapping.bindIn != mapping.bindOut || !evt.evt.key.repeat)) // prevent repeat key events from changing toggle states
							{
								matched = (evt.evt.key.keysym.sym == mapping.keycode &&
										   evt.evt.key.keysym.mod == mapping.modifier);
							}
							else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == mapping.keycode);
							}
							else if ((mapping.bindIn == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (mapping.bindIn == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = (evt.evt.jbutton.which == mapping.device &&
										   evt.evt.jbutton.button == mapping.keycode);
							}
						}
						else { // stateActive
							if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_KEYDOWN) ||
								(mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_KEYUP))
							{
								matched = (evt.evt.key.keysym.sym == mapping.keycode &&
										   evt.evt.key.keysym.mod == mapping.modifier);
								// TODO: maybe need to look for sym and mod keys separately here and make state inactive for either one
							}
							else if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_MOUSEBUTTONDOWN) ||
									 (mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_MOUSEBUTTONUP))
							{
								matched = (evt.evt.button.button == mapping.keycode);
							}
							else if ((mapping.bindOut == Bind_Down_T && evt.evt.type == SDL_JOYBUTTONDOWN) ||
									 (mapping.bindOut == Bind_Up_T   && evt.evt.type == SDL_JOYBUTTONUP))
							{
								matched = (evt.evt.jbutton.which == mapping.device &&
										   evt.evt.jbutton.button == mapping.keycode);
							}
						}

						// found a new active state mapping with this event
						if (matched && !stateActive) {
							MappedState ms{};
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
}


void InputSystem::mapFrameMotion(const UpdateInfo& ui)
{
	float inverseWindowWidth  = 1.0f / app->getPrimaryWindow().width;
	float inverseWindowHeight = 1.0f / app->getPrimaryWindow().height;
	bool relativeMode = relativeMouseModeActive();

	// reset previous frame's relative motion
	for (auto& motion : m_frameMappedInput.motion) {
		motion.relRaw = 0;
		motion.relMapped = 0.0f;
	}

	// process all motion events, these go into AxisMotion struct
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
				auto& mouseX = m_frameMappedInput.motion[0];
				auto& mouseY = m_frameMappedInput.motion[1];
				
				mouseX.posRaw = motionEvt.evt.motion.x;
				mouseX.relRaw += motionEvt.evt.motion.xrel;
				mouseX.posMapped = mouseX.posRaw * inverseWindowWidth;
				mouseX.relMapped += motionEvt.evt.motion.xrel * inverseWindowWidth;

				mouseY.posRaw = motionEvt.evt.motion.y;
				mouseY.relRaw += motionEvt.evt.motion.yrel;
				mouseY.posMapped = mouseY.posRaw * inverseWindowHeight;
				mouseY.relMapped += motionEvt.evt.motion.yrel * inverseWindowHeight;
				break;
			}
			case SDL_JOYAXISMOTION: {
				auto it = std::find_if(m_frameMappedInput.motion.begin(), m_frameMappedInput.motion.end(),
					[&motionEvt](const AxisMotion& m){
						return (m.device == motionEvt.evt.jaxis.which &&
								m.axis == motionEvt.evt.jaxis.axis);
					});

				if (it != m_frameMappedInput.motion.end()) {
					auto& motion = *it;
					auto newRaw = motionEvt.evt.jaxis.value;
					motion.relRaw = newRaw - motion.posRaw;
					motion.posRaw = newRaw;
					
					motion.posMapped = motion.posRaw * JOYSTICK_INVERSE_MAX_RAW;
					motion.relMapped = motion.relRaw * JOYSTICK_INVERSE_MAX_RAW;
				}
				break;
			}
		}
	}

	// erase up to the last event for this frame
	m_popMotionEvents.erase(m_popMotionEvents.begin(), m_popMotionEvents.begin() + frameSize);

	// all AxisMotion is aggregated for frame, now map to active InputMappings
	for (auto& motion : m_frameMappedInput.motion) {		// for each axis
		bool matched = false;
		bool isMouse = (motion.device == 0);

		for (const auto& ac : m_activeInputContexts) {		// for each active context
			// skip contexts that aren't active
			if (!ac.active) { continue; }

			auto& context = m_inputContexts[ac.contextId];

			for (Id_T mappingId : context.inputMappings) {	// check all input mappings for a match
				const auto& mapping = m_inputMappings[mappingId];

				// check for axis mappings
				matched = (mapping.type == Axis_T &&
						   mapping.device == motion.device &&
						   mapping.axis == motion.axis &&
						   ((mapping.relativeMotion == 1) == relativeMode) &&
						   (motion.relRaw != 0 || !relativeMode));
				if (matched) {
					// found a matching axis mapping for the event, apply mapping parameters
					if (isMouse) {
						motion.relMapped *= mapping.sensitivity * (mapping.invert == 1 ? -1.0f : 1.0f);
					}
					else {
						// "curved" relative is (curve(posMapped) - curve(posMapped-relMapped)), not curve(relMapped)
						//mapping.curve
						//mapping.curvature
						//mapping.deadzone
						//mapping.saturationX
						//mapping.saturationY
						//mapping.slider
						motion.relMapped *= (mapping.invert == 1 ? -1.0f : 1.0f);
					}
					
					MappedAxis ma{};
					ma.mappingId = mappingId;
					ma.inputMapping = &mapping;
					ma.axisMotion = &motion;

					m_frameMappedInput.axes.push_back(std::move(ma));
					break; // skip checking the rest of the mappings
				}
			}

			if (matched || // found a mapping, move on to the next input event
				(context.options[EatMouseEvents_T] && isMouse) || // didn't find a match, check if this context eats input events or passes them down
				(context.options[EatJoystickEvents_T] && !isMouse))
			{
				break;
			}
		}
	}
}

void InputSystem::handleInputAction(Id_T mappingId, FrameMappedInput& mappedInput,
									std::function<bool(MappedAction&, InputContext&)> callback)
{
	InputMapping mapping = m_inputMappings[mappingId];
	assert(mapping.type == Action_T && "wrong handler for mapping type");

	for (auto& mi : mappedInput.actions) {
		if (mi.mappingId == mappingId && !mi.handled) {
			auto& context = m_inputContexts[mi.inputMapping->contextId];
			mi.handled = callback(mi, context);
		}
	}
}

void InputSystem::handleInputState(Id_T mappingId, FrameMappedInput& mappedInput,
								   std::function<bool(MappedState&, InputContext&)> callback)
{
	InputMapping mapping = m_inputMappings[mappingId];
	assert(mapping.type == State_T && "wrong handler for mapping type");

	for (auto& mi : mappedInput.states) {
		if (mi.mappingId == mappingId && !mi.handled) {
			auto& context = m_inputContexts[mi.inputMapping->contextId];
			mi.handled = callback(mi, context);
		}
	}
}

void InputSystem::handleInputAxis(Id_T mappingId, FrameMappedInput& mappedInput,
								  std::function<bool(MappedAxis&, InputContext&)> callback)
{
	InputMapping mapping = m_inputMappings[mappingId];
	assert(mapping.type == Axis_T && "wrong handler for mapping type");

	for (auto& mi : mappedInput.axes) {
		if (mi.mappingId == mappingId && !mi.handled) {
			auto& context = m_inputContexts[mi.inputMapping->contextId];
			mi.handled = callback(mi, context);
		}
	}
}


bool InputSystem::handleEvent(const SDL_Event& event)
{
	bool handled = false;
	auto timestamp = Timer::queryCounts();

	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			if (event.key.repeat == 0) {
				/*SDL_Log("key event=%d: state=%d: key=%d: repeat=%d: realTime=%lu\n",
						event.type, event.key.state, event.key.keysym.scancode, event.key.repeat, timestamp);*/

				m_eventsQueue.push({ timestamp, std::move(event), Event_Keyboard_T, {} });
			}
			handled = true;
			break;
		}

		case SDL_TEXTEDITING: {
			/*SDL_Log("key event=%d: text=%s: length=%d: start=%d: windowID=%d: realTime=%lu\n",
					event.type, event.edit.text, event.edit.length, event.edit.start, event.edit.windowID, timestamp);*/

			m_eventsQueue.push({ timestamp, std::move(event), Event_TextInput_T, {} });

			handled = true;
			break;
		}
		case SDL_TEXTINPUT: {
			/*SDL_Log("key event=%d: text=%s: windowID=%d: realTime=%lu\n",
					event.type, event.text.text, event.text.windowID, timestamp);*/

			m_eventsQueue.push({ timestamp, std::move(event), Event_TextInput_T, {} });

			handled = true;
			break;
		}

		case SDL_MOUSEMOTION: {
			/*SDL_Log("mouse motion event=%d: which=%d: state=%d: window=%d: x,y=%d,%d: xrel,yrel=%d,%d: realTime=%lu\n",
					event.type, event.motion.which, event.motion.state, event.motion.windowID,
					event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, timestamp);*/
			m_motionEventsQueue.push({ timestamp, std::move(event), Event_Mouse_T, {} });
			handled = true;
			break;
		}
		case SDL_JOYAXISMOTION:
			/*SDL_Log("joystick motion event=%d: which=%d: axis=%d: value=%d: realTime=%lu\n",
					event.type, event.jaxis.which, event.jaxis.axis, event.jaxis.value, timestamp);*/
		case SDL_JOYBALLMOTION:
		case SDL_JOYHATMOTION: {
			m_motionEventsQueue.push({ timestamp, std::move(event), Event_Joystick_T, {} });
			handled = true;
			break;
		}

		case SDL_MOUSEWHEEL: {
			SDL_Log("mouse wheel event=%d: which=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.wheel.which, event.wheel.windowID,
					event.wheel.x, event.wheel.y, timestamp);

			m_eventsQueue.push({ timestamp, std::move(event), Event_Mouse_T, {} });
			handled = true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			SDL_Log("mouse button event=%d: which=%d: button=%d: state=%d: clicks=%d: window=%d: x,y=%d,%d: realTime=%lu\n",
					event.type, event.button.which, event.button.button, event.button.state,
					event.button.clicks, event.button.windowID, event.button.x, event.button.y, timestamp);

			m_eventsQueue.push({ timestamp, std::move(event), Event_Mouse_T, {} });
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
			
			m_eventsQueue.push({ timestamp, std::move(event), Event_Joystick_T, {} });
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


	// Get number of input devices
	int numJoysticks = SDL_NumJoysticks();
	m_joysticks.reserve(numJoysticks);
	m_frameMappedInput.axes.reserve(numJoysticks * 3 + 2);   // number of joysticks * 3 plus 2 for mouse
	m_frameMappedInput.motion.reserve(numJoysticks * 3 + 2); // assumes 3 axes per joystick device on average
	
	// Initialize the mouse
	int numPointingDevices = 1; //SDL_GetNumInputDevices();
	// mouse is always index 0 and 1 (x and y) in AxisMotion array
	{ // this scope would be a loop in future SDL versions that support multiple mice
		int numAxes = 2;
		for (int axis = 0; axis < numAxes; ++axis) {
			AxisMotion m{};
			m.device = 0;
			m.deviceName = "mouse";//SDL_GetInputDeviceName();
			m.axis = axis;
			m_frameMappedInput.motion.push_back(std::move(m));
		}
	}
	
	// Initialize the joysticks
	for (int j = 0; j < numJoysticks; ++j) {
		// Open joystick
		SDL_Joystick* joy = SDL_JoystickOpen(j);
		
		if (joy != nullptr) {
			m_joysticks.push_back(joy);

			// for each axis, create an AxisMotion struct
			int numAxes = SDL_JoystickNumAxes(joy);
			for (int axis = 0; axis < numAxes; ++axis) {
				AxisMotion m{};
				m.device = SDL_JoystickInstanceID(joy);
				m.deviceName = SDL_JoystickName(joy);
				m.axis = axis;
				
				m_frameMappedInput.motion.push_back(std::move(m));
			}

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


Id_T InputSystem::getInputMappingHandle(const char* name, Id_T contextId) const
{
	const auto& context = m_inputContexts[contextId];

	for (Id_T m : context.inputMappings) {
		const auto& mapping = m_inputMappings[m];
		if (strncmp(mapping.name, name, sizeof(mapping.name)) == 0) {
			return mapping.mappingId;
		}
	}
	return NullId_T;
}


size_t InputSystem::findActiveState(Id_T mappingId) const
{
	for (size_t s = 0; s < m_frameMappedInput.states.size();  ++s) {
		if (m_frameMappedInput.states[s].mappingId.value == mappingId.value) { return s; }
	}
	return -1;
}


Id_T InputSystem::createContext(uint16_t optionsMask, uint8_t priority, bool makeActive)
{
	//auto f = tss_([optionsMask](ThreadSafeState& tss_) {
	//	return tss_.m_inputContexts.emplace(optionsMask);
	//});
	//return f;

	auto contextId = m_inputContexts.emplace(optionsMask);
	m_activeInputContexts.push_back({ contextId, makeActive, priority, {} });
	std::stable_sort(m_activeInputContexts.begin(), m_activeInputContexts.end(),
					[](const ActiveInputContext& i, const ActiveInputContext& j) {
						return i.priority < j.priority;
					});

	return contextId;
}


bool InputSystem::setContextActive(Id_T contextId, bool active)
{
	if (m_inputContexts.isValid(contextId)) {
		for (auto& ac : m_activeInputContexts) {
			if (ac.contextId == contextId) {
				ac.active = active;
				break;
			}
		}
		return true;
	}
	return false;
}


Id_T InputSystem::getInputContextHandle(const char* name) const
{
	for (const auto& i : m_inputContexts.getItems()) {
		if (strncmp(i.name, name, sizeof(i.name)) == 0) {
			return i.contextId;
		}
	}
	return NullId_T;
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
	if (m_callbacks.capacity() > RESERVE_INPUTSYSTEM_CALLBACKS) {
		SDL_Log("check RESERVE_INPUTSYSTEM_CALLBACKS: original=%d, highest=%d", RESERVE_INPUTSYSTEM_CALLBACKS, m_callbacks.capacity());
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