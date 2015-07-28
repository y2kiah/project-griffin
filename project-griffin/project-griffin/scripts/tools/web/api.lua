local ffi = require("ffi")
local lfs = require("lfs")
local JSON = require("JSON")
local griffin = require("tools/GriffinToolsApi")
JSON.strictTypes = true

local C = ffi.C

local data = {}

-- Tools API provided as REST interface

-- griffin_tools_importMesh
if (cgilua.QUERY.method == "griffin_tools_importMesh" and
	cgilua.QUERY.filename)
then
	--lfs.currentdir() is ...\\project-griffin\\project-griffin\\project-griffin\\scripts\\tools\\web
	data = ffi.C.griffin_tools_importMesh("../../../" .. cgilua.QUERY.filename)

-- griffin_tools_saveMesh
elseif (cgilua.QUERY.method == "griffin_tools_saveMesh" and
	cgilua.QUERY.mesh and
	cgilua.QUERY.filename)
then
	data = ffi.C.cgilua.griffin_tools_saveMesh(cgilua.QUERY.mesh, cgilua.QUERY.filename)

-- griffin_tools_convertMesh
elseif (cgilua.QUERY.method == "griffin_tools_convertMesh" and
	cgilua.QUERY.sourceFilename and
	cgilua.QUERY.destFilename)
then
	data = ffi.C.cgilua.griffin_tools_convertMesh(cgilua.QUERY.sourceFilename, cgilua.QUERY.destFilename)
end


cgilua.contentheader("application", "json")
cgilua.put(JSON:encode(data))