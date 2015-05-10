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
	//////////
	typedef struct {
		uint8_t		type;			//<! type of this mapping
		uint8_t		bindIn;			//<! event to start the action or state
		uint8_t		bindOut;		//<! event to end the state
		uint8_t		curve;			//<! curve type of axis
		uint32_t	device;			//<! instanceID of the device, comes through event "which"
		uint32_t	keycode;		//<! keyboard virtual key code, mouse or joystick button
		uint16_t	modifier;		//<! keyboard modifier, SDL_Keymod, defaults to 0 (KMOD_NONE)
		uint8_t		mouseWheel;		//<! 0=false, 1=true is a mouse wheel binding
		uint8_t		axis;			//<! index of the joystick axis
		uint8_t		clicks;			//<! number of mouse clicks for binding (2==double-click)
		uint8_t		deadzone;		//<! deadzone for axis
		uint8_t		curvature;		//<! curvature of axis
		uint8_t		saturationX;	//<! saturation of the x axis
		uint8_t		saturationY;	//<! saturation of the y axis
		uint8_t		numAxes;		//<! 1 for most joysticks, 2 for mouse and balls
		uint8_t		relativeMotion;	//<! 0=false, 1=true motion is relative not absolute
		uint8_t		invert;			//<! 0=false, 1=true axis is inverted
		uint8_t		slider;			//<! 0=false, 1=true axis is a slider
		uint64_t	mappingId;		//<! the id handle of this mapping
		uint64_t	contextId;		//<! the id handle of the context
		char		name[32];		//<! display name of the mapping
	} griffin_InputMapping;

	
	uint64_t griffin_input_createContext(uint16_t optionsMask, uint8_t priority, const char name[32], bool makeActive);

	
	bool griffin_input_setContextActive(uint64_t context, bool active);

	
	uint64_t griffin_input_createInputMapping(const char name[32], uint64_t context);

	
	griffin_InputMapping* griffin_input_getInputMapping(uint64_t mapping);

	//typedef void(*CallbackFunc_T)()

	
	void griffin_input_registerCallback();


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

	function setKeyBinding(contextName, mapping, bindings)
		for b = 1,#bindings do
			local binding = bindings[b]
			if (binding.context == contextName and
				binding.name == ffi.string(mapping.name))
			then
				if binding.device ~= nil then   mapping.device = binding.device end
				if binding.keycode ~= nil then  mapping.keycode = binding.keycode end
				if binding.modifier ~= nil then mapping.modifier = binding.modifier end
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
			setKeyBinding(contextName, mapping, config.inputMappings.actions)
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

			--print(ffi.string(mapping.name))
			--print(mapping.bindIn)
			--print(mapping.bindOut)

			-- set key bindings from inputs.json
			setKeyBinding(contextName, mapping, config.inputMappings.states)
		end

		-- id is 64-bit boxed cdata, unsuitable for use as a table key, use tostring(id) to use as a key
		--print(contextId)
	end

	-- make ingame context active
	C.griffin_input_setContextActive(contextMap["ingame"], true)
	C.griffin_input_setContextActive(contextMap["playerfps"], true)

	-- build Lua table for the input system
	_G["InputSystem"] = {
		["config"] = config
	}
end
