local ffi = require("ffi")

ffi.cdef[[

	enum {
		CONTEXT_OPTION_CAPTURE_TEXT_INPUT  = 0,		//<! true if text input events should be captured by this context
		CONTEXT_OPTION_RELATIVE_MOUSE_MODE = 1,		//<! true for relative mouse mode vs. regular "GUI" mode
		CONTEXT_OPTION_SHOW_MOUSE_CURSOR   = 2,		//<! true to show cursor specified by m_cursorIndex
		CONTEXT_OPTION_EAT_KEYBOARD_EVENTS = 4,		//<! true to eat all keyboard events, preventing pass-down to lower contexts
		CONTEXT_OPTION_EAT_MOUSE_EVENTS    = 8,		//<! prevent mouse events from passing down
		CONTEXT_OPTION_EAT_JOYSTICK_EVENTS = 16		//<! prevent joystick events from passing down
	};

	enum {
		MAPPING_TYPE_ACTION = 0,
		MAPPING_TYPE_STATE  = 1,
		MAPPING_TYPE_AXIS   = 2
	};

	enum {
		MAPPING_BIND_DOWN   = 0,
		MAPPING_BIND_UP     = 1,
		MAPPING_BIND_TOGGLE = 2
	};
	
	enum {
		MAPPING_CURVE_LINEAR = 0,
		MAPPING_CURVE_SCURVE = 1
	};

	//////////
	// Input mappings map raw input events or position data to Actions, States or Ranges. These
	// are high-level game input events obtained from configuration data through Lua.
	//	Actions: Single-time events, not affected by key repeat, mapped to a single input event
	//			 e.g. {keydown-G} or {keyup-G}.
	//	States:  Binary on/off flag mapped to two input events e.g. {on:keydown-G, off:keyup-G}
	//			 or {on:keydown-G, off:keydown-G}. The first example would be typical of WASD
	//			 type movement state, the second example would behave like a toggle.
	//	Axis:	 Uses position information of joysticks, throttles, rudder pedals, head
	//			 tracking gear, even mouse movement if desired.
	// Layout equivalent to InputMapping
	//////////
	typedef struct {
		uint8_t						type;				//<! type of this mapping
		uint8_t						bindIn;				//<! event to start the action or state
		uint8_t						bindOut;			//<! event to end the state
		uint8_t						curve;				//<! curve type of axis
		uint32_t					device;				//<! instanceID of the device, comes through event "which"
		// keyboard, mouse button, joystick button, mouse wheel events
		uint32_t					keycode;			//<! keyboard virtual key code, mouse or joystick button
		uint16_t					modifier;			//<! keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
		uint8_t						mouseWheel;			//<! 0=false, 1=true is a mouse wheel binding
		uint8_t						clicks;				//<! number of mouse clicks for binding (2==double-click)
		// mouse motion, joystick motion, joystick hat motion, ball motion events
		uint8_t						axis;				//<! index of the joystick axis
		uint8_t						deadzone;			//<! deadzone for axis
		uint8_t						curvature;			//<! curvature of axis
		uint8_t						saturationX;		//<! saturation of the x axis
		uint8_t						saturationY;		//<! saturation of the y axis
		uint8_t						relativeMotion;		//<! 0=false, 1=true motion is relative not absolute
		uint8_t						invert;				//<! 0=false, 1=true axis is inverted
		uint8_t						slider;				//<! 0=false, 1=true axis is a slider
		float						sensitivity;		//<! sensitivity multiplier, mainly for mouse movement in relative mode
		// all events
		uint64_t					mappingId;			//<! the id handle of this mapping
		uint64_t					contextId;			//<! the id handle of the context
		char						name[32];			//<! display name of the mapping
	} griffin_InputMapping;

	//////////
	// Layout equivalent to MappedAction
	//////////
	typedef struct {
		uint64_t					mappingId;
		uint8_t						handled;			//<! flag set to 1 when event has been handled by a callback
		const griffin_InputMapping*	inputMapping;
		float						x;
		float						y;					//<! mouse clicks include normalized position here
		int32_t						xRaw;
		int32_t						yRaw;
	} griffin_MappedAction;

	//////////
	// Layout equivalent to MappedState
	//////////
	typedef struct {
		uint64_t					mappingId;
		uint8_t						handled;			//<! flag set to 1 when event has been handled by a callback
		const griffin_InputMapping*	inputMapping;
		double						totalMs;			//<! total millis the state has been active
		int64_t						startCounts;		//<! clock counts when state began
		int64_t						totalCounts;		//<! currentCounts - startCounts + countsPerTick
		int32_t						startFrame;			//<! frame number when state began
		int32_t						totalFrames;		//<! currentFrame - startFrame + 1
	} griffin_MappedState;

	//////////
	// Layout equivalent to AxisMotion
	//////////
	typedef struct {
		uint32_t					device;				//<! instanceID of the device that owns this axis, mouse is always 0 (x) and 1 (y)
		uint8_t						axis;				//<! axis number on the device
		float						posMapped;			//<! absolute position of axis mapped to curve
		float						relMapped;			//<! relative motion of the axis since last frame mapped to curve
		int32_t						posRaw;				//<! raw value from device, not normalized or mapped to curve, may be useful but use posMapped by default
		int32_t						relRaw;				//<! relative raw value of the axis
		const char *				deviceName;			//<! name of the device
	} griffin_AxisMotion;

	//////////
	// Layout equivalent to MappedAxis
	//////////
	typedef struct {
		uint64_t					mappingId;
		uint8_t						handled;			//<! flag set to 1 when event has been handled by a callback
		const griffin_InputMapping*	inputMapping;
		const griffin_AxisMotion *	axisMotion;
	} griffin_MappedAxis;

	//////////
	// No equivalent layout on the C++ side
	//////////
	typedef struct {
		int16_t						actionsSize;
		int16_t						statesSize;
		int16_t						axesSize;
		int16_t						axisMotionSize;
		int16_t						textInputLength;
		griffin_MappedAction *		actions;			//<! Actions mapped to an active InputMapping for the frame
		griffin_MappedState	*		states;				//<! States mapped to an active InputMapping for the frame
		griffin_MappedAxis *		axes;				//<! Axes mapped to an active InputMapping for the frame
		griffin_AxisMotion *		axisMotion;			//<! Holds accumulated motion for the mouse and joysticks
		const wchar_t *				textInput;			//<! Text input buffer
	} griffin_FrameMappedInput;

	typedef void(*Callback_T)(griffin_FrameMappedInput*);


	// Functions

	
	uint64_t griffin_input_createContext(uint16_t optionsMask, uint8_t priority, const char name[32], bool makeActive);

	
	bool griffin_input_setContextActive(uint64_t context, bool active);

	
	uint64_t griffin_input_createInputMapping(const char name[32], uint64_t context);

	
	griffin_InputMapping* griffin_input_getInputMapping(uint64_t mapping);

	
	uint64_t griffin_input_registerCallback(int priority, Callback_T callbackFunc);

	
	bool griffin_input_removeCallback(uint64_t callback);


]]
local C = ffi.C;

-- function invoked by InputSystem initialization
function initInputSystem()
	local config = {}

	-- read input contexts
	local fr = assert(io.open("data/config/inputcontexts.json", "r"))
	local inputContextsContent = fr:read("*all")
	fr:close()

	config.inputContexts = JSON:decode(inputContextsContent)

	-- read input mappings
	fr = assert(io.open("data/config/inputs.json", "r"))
	local inputMappingsContent = fr:read("*all")
	fr:close()

	config.inputMappings = JSON:decode(inputMappingsContent)
	--print(config.inputMappings)

	function setInputBinding(contextName, mapping, bindings)
		for b = 1,#bindings do
			local binding = bindings[b]
			if (binding.context == contextName and
				binding.name == ffi.string(mapping.name))
			then
				if binding.device ~= nil then		mapping.device = binding.device end
				if binding.keycode ~= nil then		mapping.keycode = binding.keycode end
				if binding.modifier ~= nil then		mapping.modifier = binding.modifier end

				if binding.axis ~= nil then			mapping.axis = binding.axis end
				if binding.sensitivity ~= nil then	mapping.sensitivity = binding.sensitivity end
			end
		end
	end

	local contextMap = {}

	-- create contexts from inputcontexts.json
	for contextName,context in pairs(config.inputContexts) do
		print(contextName)

		local contextOptions = 0
		local contextId = C.griffin_input_createContext(contextOptions, context.priority, contextName, false)
		contextMap[contextName] = contextId;
		print(tostring(contextId))

		local contextMappings = {}

		-- create action mappings from inputcontexts.json
		for m = 1,#context.actions do
			local action = context.actions[m]
			
			local mappingId = C.griffin_input_createInputMapping(action.name, contextId)
			local mapping = C.griffin_input_getInputMapping(mappingId)

			contextMappings[action.name] = mapping

			mapping.type = C.MAPPING_TYPE_ACTION
			if (action.bind == "down") then
				mapping.bindIn = C.MAPPING_BIND_DOWN

			elseif (action.bind == "up") then
				mapping.bindIn = C.MAPPING_BIND_UP
			end

			-- set key bindings from inputs.json
			setInputBinding(contextName, mapping, config.inputMappings.actions)
		end

		-- create state mappings from inputcontexts.json
		for m = 1,#context.states do
			local state = context.states[m]

			local mappingId = C.griffin_input_createInputMapping(state.name, contextId)
			local mapping = C.griffin_input_getInputMapping(mappingId)

			contextMappings[state.name] = mapping

			mapping.type = C.MAPPING_TYPE_STATE
			if (state.bind == "down") then
				mapping.bindIn = C.MAPPING_BIND_DOWN
				mapping.bindOut = C.MAPPING_BIND_UP

			elseif (state.bind == "up") then
				mapping.bindIn = C.MAPPING_BIND_UP
				mapping.bindOut = C.MAPPING_BIND_DOWN
				-- states with an "up" binding should be active by default, could do that here

			elseif (state.bind == "toggle") then
				mapping.bindIn = C.MAPPING_BIND_DOWN
				mapping.bindOut = C.MAPPING_BIND_DOWN
			end

			-- set key bindings from inputs.json
			setInputBinding(contextName, mapping, config.inputMappings.states)
		end

		-- create state mappings from inputcontexts.json
		for m = 1,#context.axes do
			local axis = context.axes[m]

			local mappingId = C.griffin_input_createInputMapping(axis.name, contextId)
			local mapping = C.griffin_input_getInputMapping(mappingId)

			contextMappings[axis.name] = mapping
			mapping.type = C.MAPPING_TYPE_AXIS

			-- set Axis bindings from inputs.json
			setInputBinding(contextName, mapping, config.inputMappings.axes)
		end
	end

	-- make ingame context active
	C.griffin_input_setContextActive(contextMap["ingame"], true)
	C.griffin_input_setContextActive(contextMap["playerfps"], true)

	local callbackHandle = C.griffin_input_registerCallback(0, frameInputHandler)

	-- build Lua table for the input system
	_G["InputSystem"] = {
		config = config,
		callbackHandle = callbackHandle
	}
end

function frameInputHandler(frameMappedInput)
	local mi = frameMappedInput
	local luaMappedInput = {}
	
	function copyMappedInputToLuaTable(size, mappedInputs, mappingType)
		for i = 0, size-1 do
			local mappedInput = mappedInputs[i]
			
			if mappedInputs == mi.actions then
				print("action " .. ffi.string(mappedInput.inputMapping.name) .. " handled")
			elseif mappedInputs == mi.states then
				print("state " .. ffi.string(mappedInput.inputMapping.name) .. " handled active")
			elseif mappedInputs == mi.axes then
				print("axis " .. ffi.string(mappedInput.inputMapping.name) .. " handled motion, posMapped=" .. mappedInput.axisMotion.posMapped)
			end

			-- ids are 64-bit boxed cdata, unsuitable for use as a table key, converting to string to use as a key
			local context = tostring(mappedInput.inputMapping.contextId)
			local mapping = tostring(mappedInput.inputMapping.mappingId)
			if luaMappedInput[context] == nil then luaMappedInput[context] = {} end

			luaMappedInput[context][mapping] = {
				mappingType = mappingType,
				mappedInput = mappedInput
			}
		end
	end

	copyMappedInputToLuaTable(mi.actionsSize, mi.actions, "action")
	copyMappedInputToLuaTable(mi.statesSize, mi.states, "state")
	copyMappedInputToLuaTable(mi.axesSize, mi.axes, "axis")

	-- dispatch to all Lua callbacks from here
end
