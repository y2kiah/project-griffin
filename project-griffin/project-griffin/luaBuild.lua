local ffi = require("ffi")

ffi.cdef[[
#pragma pack(push)
#pragma pack(1)
struct WIN32_FIND_DATAA {
	uint32_t dwFileAttributes;
	uint64_t ftCreationTime;
	uint64_t ftLastAccessTime;
	uint64_t ftLastWriteTime;
	struct {
		union {
			uint64_t packed;
			struct {
				uint32_t high;
				uint32_t low;
			};
		};
	} nFileSize;
	uint32_t dwReserved[2];
	char cFileName[260];
	char cAlternateFileName[14];
};
#pragma pack(pop)

void* FindFirstFileA(const char* pattern, struct WIN32_FIND_DATAA* fd);
bool  FindNextFileA(void* ff, struct WIN32_FIND_DATAA* fd);
bool  FindClose(void* ff);
]]

local WIN32_FIND_DATAA = ffi.typeof("struct WIN32_FIND_DATAA")
local INVALID_HANDLE = ffi.cast("void*", -1)
local FILE_ATTRIBUTE_DIRECTORY = 16

--[[
Get all files in a directory that match the provided pattern, pass wildcard "*" to match all files
and directories under the path.
]]
getDirectoryFiles = function(initialPath, pattern, recursive)
	if (not initialPath:sub(-1):find("[\\/]")) then
		initialPath = initialPath .. "/"
	end
	
	local paths = { initialPath }
	local tFiles = {}

	repeat
		local path = table.remove(paths)

		local fd = ffi.new(WIN32_FIND_DATAA)
		local hFile = ffi.C.FindFirstFileA(path..pattern, fd)
	
		if (hFile ~= INVALID_HANDLE) then
			ffi.gc(hFile, ffi.C.FindClose)
		
			repeat
				fd.nFileSize.low, fd.nFileSize.high = fd.nFileSize.high, fd.nFileSize.low

				local filename = ffi.string(fd.cFileName)
				local directory = (bit.band(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ~= 0)

				tFiles[path..filename] = {
					filename = filename,
					path = path,
					attrib = fd.dwFileAttributes,
					creationTime = fd.ftCreationTime,
					lastAccessTime = fd.ftLastAccessTime,
					lastWriteTime = fd.ftLastWriteTime,
					size = fd.nFileSize.packed,
					directory = directory
				}
				if (recursive and directory and
					filename ~= "." and filename ~= "..")
				then
					table.insert(paths, path..filename.."/")
				end
			until not ffi.C.FindNextFileA(hFile, fd)

			ffi.C.FindClose(ffi.gc(hFile, nil))
		end
	until table.getn(paths) == 0

	return tFiles
end

--[[
string recipes http://lua-users.org/wiki/StringRecipes
move these into another file
]]
function string.startsWith(str, startsWith)
	return (string.sub(str, 1, string.len(startsWith)) == startsWith)
end

function string.endsWith(str, endsWith)
	return (endsWith == "" or string.sub(str, -string.len(endsWith)) == endsWith)
end
----------

local shaderSourcePath = "source/shaders/"
local shaderBuildPath  = "data/shaders/"

local files = getDirectoryFiles(shaderSourcePath, "*", true)

-- loop over all shaders with .glsl extension, do not copy .glsli files
for file, attribs in pairs(files) do
	if (string.endsWith(file, ".glsl")) then
		print("building shader: "..file)

		-- read original shader source
		local fr = assert(io.open(file, "r"))
		local content = fr:read("*all")
		fr:close()

		-- look for occurrences of #include lines and replace with included file
		local newContent = string.gsub(content, "#include \"(%C+)\"",
			function(includeFile)
				local fInc = assert(io.open(includeFile, "r"))
				local includeContent = fInc:read("*all")
				fInc:close()

				return includeContent
			end)

		-- create shader in build path with new content
		local newPath = string.gsub(file, shaderSourcePath, shaderBuildPath, 1)
		local fw = assert(io.open(newPath, "w+"))
		fw:write(newContent)
		fw:close()
	end
end