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

local files = getDirectoryFiles("source/shaders/", "*", true)
print(files)

for k, v in pairs(files) do
	if (string.find(k, ".glsl")) then
		local f = assert(io.open(k, "r"))
		local t = f:read("*all")
		--print(t)
		f:close()
	end
end