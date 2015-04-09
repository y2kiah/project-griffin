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

-- function invoked by InputSystem initialization
function loadInputSystemConfig()
	print(config)
end

_G["config"] = config