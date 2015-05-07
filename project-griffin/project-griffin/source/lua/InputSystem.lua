local ffi = require("ffi")

ffi.cdef[[
#include "source/api/InputSystemApi.h"
]]

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

	-- create contexts
	for k,v in pairs(config.inputContexts) do
		print(k)
		print(v)
		local id = ffi.C.griffin_input_createContext();
		-- id is 64-bit boxed cdata, unsuitable for use as a table key, use tostring(id) to use as a key
		print(id)
	end

	-- build Lua table for the input system
	_G["InputSystem"] = {
		["config"] = config
	}
end
