local ffi = require("ffi")
local lfs = require("lfs")
local JSON = require("JSON")
local griffin = require("tools/GriffinToolsApi")
JSON.strictTypes = true

local C = ffi.C

local data = {}
local method = cgilua.servervariable("REQUEST_METHOD")

local cwd = lfs.currentdir()
-- cwd is ...\\project-griffin\\project-griffin\\project-griffin\\scripts\\tools\\web
lfs.chdir("../../../"); -- go to executable directory

-- Tools API provided as REST interface
if method == "GET" then
	-- griffin_tools_importMesh
	if (cgilua.QUERY.method == "griffin_tools_importMesh" and
		cgilua.QUERY.filename)
	then
		local meshId = ffi.C.griffin_tools_importMesh("data/"..cgilua.QUERY.filename,
													  (cgilua.QUERY.preTransformVertices == "true"),
													  (cgilua.QUERY.flipUVs == "true"))
		data = tonumber(meshId)

	-- griffin_tools_saveMesh
	elseif (cgilua.QUERY.method == "griffin_tools_saveMesh" and
		cgilua.QUERY.mesh and
		cgilua.QUERY.filename)
	then
		local result = ffi.C.griffin_tools_saveMesh(tonumber(cgilua.QUERY.mesh), -- TODO: this is not being accepted after import passed from client
													"data/"..cgilua.QUERY.filename)
		data = result

	-- griffin_tools_convertMesh
	elseif (cgilua.QUERY.method == "griffin_tools_convertMesh" and
		cgilua.QUERY.sourceFilename and
		cgilua.QUERY.destFilename)
	then
		local result = ffi.C.griffin_tools_convertMesh("data/"..cgilua.QUERY.sourceFilename,
													   "data/"..cgilua.QUERY.destFilename,
													   (cgilua.QUERY.preTransformVertices == "true"),
													   (cgilua.QUERY.flipUVs == "true"))
		data = result -- TODO: this is throwing a JSON encoding error
	end

end

lfs.chdir(cwd)

cgilua.contentheader("application", "json")
cgilua.put(JSON:encode(data))