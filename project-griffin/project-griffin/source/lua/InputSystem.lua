local ffi = require("ffi")

ffi.cdef[[
#include "source/api/InputSystemApi.h"
]]
local C = ffi.C

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
		local contextOptions = 0
		local contextId = C.griffin_input_createContext(contextOptions, context.priority, contextName, false)
		
		contextMap[contextName] = {
			contextId = contextId, -- ids are 64-bit boxed cdata, unsuitable for use as table keys, converting to string to use as a key
			mappings = {}
		}

		local contextMappings = contextMap[contextName].mappings

		-- create action mappings from inputcontexts.json
		for m = 1,#context.actions do
			local action = context.actions[m]
			
			local mappingId = C.griffin_input_createInputMapping(action.name, contextId)
			local mapping = C.griffin_input_getInputMapping(mappingId)

			contextMappings[action.name] = {
				mapping = mapping,
				mappingId = mappingId
			}

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

			contextMappings[state.name] = {
				mapping = mapping,
				mappingId = mappingId
			}

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

			contextMappings[axis.name] = {
				mapping = mapping,
				mappingId = mappingId
			}

			mapping.type = C.MAPPING_TYPE_AXIS
			if axis.relativeMotion ~= nil then mapping.relativeMotion = (axis.relativeMotion and 1 or 0) end

			-- set Axis bindings from inputs.json
			setInputBinding(contextName, mapping, config.inputMappings.axes)
		end
	end

	-- register the Lua callback for the input system, all Lua handlers are dispatched from this callback
	local callbackHandle = C.griffin_input_registerCallback(0, frameInputHandler)

	-- build global InputSystem class
	-- variables
	_G["InputSystem"] = {
		config = config,
		callbackHandle = callbackHandle,
		inputHandlers = {},
		contextMap = contextMap
	}
	-- methods
	function InputSystem:addInputHandler(callback, priority)
		priority = priority or 100

		local handlerEntry = {
			priority = priority,
			callback = callback
		}
		setmetatable(handlerEntry, { __mode = "v" }) -- make the values weak references

		table.insert(self.inputHandlers, handlerEntry)
		table.sort(self.inputHandlers, function(a,b)
			return a.priority < b.priority
		end)
	end

	function InputSystem:removeInputHandler(callback)
	end

	function InputSystem:setContextActive(contextName, active)
		if active == nil then active = true end

		local context = self.contextMap[contextName]
		if context == nil then return false end
			
		return C.griffin_input_setContextActive(context.contextId, active)
	end

	function InputSystem:setRelativeMouseMode(active)
		if active == nil then active = true end
			
		C.griffin_input_setRelativeMouseMode(active);
	end

	function InputSystem:getRelativeMouseMode()
		return C.griffin_input_relativeMouseModeActive()
	end

	function InputSystem:handleInput(contextName, mappingName, mappedInputs, handlerFunc)
		local context = InputSystem.contextMap[contextName]
		local mapping = context and context.mappings[mappingName]
		
		local mc = mapping and mappedInputs[tostring(context.contextId)]
		
		local mappingId = tostring(mapping.mappingId)
		local mi = mc and mc[mappingId]
		
		if mi then
			if type(handlerFunc) == "function" then
				local result = handlerFunc(mc, mi)
				local handled = (result ~= nil and result or true)
				mi.handled = handled
				if handled then mc[mappingId] = nil end
			end
		end
	end

	-- add lua callback for catchAll handler
	InputSystem:addInputHandler(catchAllHandler, 999)
end


function frameInputHandler(frameMappedInput)
	local mi = frameMappedInput
	local luaFrameInput = {
		actions = {},
		states = {},
		axes = {},
		motion = {}
	}
	
	function copyMappedInputToLuaTable(size, mappedInputs, mappingType)
		for i = 0, size-1 do
			local mappedInput = mappedInputs[i]
			
			if mappedInputs == mi.axisMotion then
				luaFrameInput.motion[mappedInput.device] = mappedInput

			else
				local context = tostring(mappedInput.inputMapping.contextId)
				local mapping = tostring(mappedInput.inputMapping.mappingId)
				luaFrameInput[mappingType][context] = luaFrameInput[mappingType][context] or {}

				luaFrameInput[mappingType][context][mapping] = mappedInput
			end
		end
	end

	copyMappedInputToLuaTable(mi.actionsSize, mi.actions, "actions")
	copyMappedInputToLuaTable(mi.statesSize, mi.states, "states")
	copyMappedInputToLuaTable(mi.axesSize, mi.axes, "axes")
	copyMappedInputToLuaTable(mi.axisMotionSize, mi.axisMotion, "motion")

	-- dispatch to all Lua callbacks from here
	for k,handler in pairs(InputSystem.inputHandlers) do
		if handler.callback ~= nil then
			handler.callback(luaFrameInput)
		end
	end
end


-- catchAll handler logs the actions that aren't handled by higher-priority handlers
function catchAllHandler(frameInput)
	for i,mc in pairs(frameInput.actions) do
		for j,mi in pairs(mc) do
			print("action " .. ffi.string(mi.inputMapping.name) .. " handled")
		end
	end
	for i,mc in pairs(frameInput.states) do
		for j,mi in pairs(mc) do
			print("state " .. ffi.string(mi.inputMapping.name) .. " handled active")
		end
	end
	for i,mc in pairs(frameInput.axes) do
		for j,mi in pairs(mc) do
			print("axis " .. ffi.string(mi.inputMapping.name) .. " handled motion, relRaw=" .. mi.axisMotion.relRaw .. ", relMapped=" .. mi.axisMotion.relMapped)
		end
	end
end