local ffi = require("ffi")
local lfs = require("lfs")
local JSON = require("JSON")
local griffin = require("tools/GriffinToolsApi")
JSON.strictTypes = true

local C = ffi.C
local relPathToData = "../../../data/"

local data = {}
local method = cgilua.servervariable("REQUEST_METHOD")

-- Tools API provided as REST interface
if method == "GET" then
	-- griffin_tools_importMesh
	if (cgilua.QUERY.method == "griffin_tools_importMesh" and
		cgilua.QUERY.filename)
	then
		--lfs.currentdir() is ...\\project-griffin\\project-griffin\\project-griffin\\scripts\\tools\\web
		local meshId = ffi.C.griffin_tools_importMesh(relPathToData..cgilua.QUERY.filename)
		data = tonumber(meshId)

	-- griffin_tools_saveMesh
	elseif (cgilua.QUERY.method == "griffin_tools_saveMesh" and
		cgilua.QUERY.mesh and
		cgilua.QUERY.filename)
	then
		local result = ffi.C.griffin_tools_saveMesh(tonumber(cgilua.QUERY.mesh),
													relPathToData..cgilua.QUERY.filename)
		data = result

	-- griffin_tools_convertMesh
	elseif (cgilua.QUERY.method == "griffin_tools_convertMesh" and
		cgilua.QUERY.sourceFilename and
		cgilua.QUERY.destFilename)
	then
		local result = ffi.C.griffin_tools_convertMesh(relPathToData..cgilua.QUERY.sourceFilename,
													   relPathToData..cgilua.QUERY.destFilename)
		data = result
	end

end

cgilua.contentheader("application", "json")
cgilua.put(JSON:encode(data))