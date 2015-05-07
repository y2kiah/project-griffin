local ffi = require("ffi")

ffi.cdef[[
void OutputDebugStringA(const char* lpOutputString);
void debug_printf(const char *fmt);
]]

debugPrint = ffi.C.OutputDebugStringA;

-- Override the print function
function print(printObj)
	local function recurse(printObj, level)
		if level > 10 then return end

		if type(printObj) == "string" then
			debugPrint(printObj)

		elseif type(printObj) == "number" then
			debugPrint(string.format("%.7g", printObj))

		elseif type(printObj) == "boolean" then
			if printObj then debugPrint("true") else debugPrint("false") end

		elseif type(printObj) == "table" then
			debugPrint("{\n")
			for k,v in pairs(printObj) do
				if (v ~= printObj and not (level > 1 and k == "_G") and
					not (k == "package"))
				then
					debugPrint(string.rep("\t",level)..k..": ")
					recurse(v, level+1)
					debugPrint("\n")
				end
			end
			debugPrint(string.rep("\t",level-1).."}")

		elseif type(printObj) == "userdata" then
			local mt = getmetatable(printObj)
			if mt then
				debugPrint("{\n"..string.rep("\t",level).."__index: ")
				recurse(mt, level+1)
				debugPrint("\n"..string.rep("\t",level-1).."}")
			else
				debugPrint(tostring(printObj))
			end
		
		elseif type(printObj) == "function" then
			debugPrint("function")

		else
			debugPrint(tostring(printObj))
		end
	end
	if printObj ~= nil then
		recurse(printObj, 1)
		debugPrint("\n")
	else
		debugPrint("nil\n")
	end
end

-- string recipes http://lua-users.org/wiki/StringRecipes
function string.startsWith(str, startsWith)
	return (string.sub(str, 1, string.len(startsWith)) == startsWith)
end

function string.endsWith(str, endsWith)
	return (endsWith == "" or string.sub(str, -string.len(endsWith)) == endsWith)
end


ffi.C.debug_printf("Hello World from Lua" .. "JIT" .. " script!")
--print(_G)

dofile("scripts/luaBuild.lua")
JSON = dofile("scripts/JSON.lua")
JSON.strictTypes = true
