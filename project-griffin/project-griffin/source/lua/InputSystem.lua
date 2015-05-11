local ffi = require("ffi")

ffi.cdef[[
#include "source/api/InputSystemApi.h"
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
