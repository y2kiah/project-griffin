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

getDirectoryFiles = function(path, pattern, recursive)
	if not path:sub(-1):find("[\\/]") then
		path = path .. "/"
	end
	
	local paths = { path .. pattern }
	local tFiles = {}

	-- for each in paths, recursive directories are pushed
	local fd = ffi.new(WIN32_FIND_DATAA)
	local hFile = ffi.C.FindFirstFileA(path .. pattern, fd)
	
	if hFile ~= INVALID_HANDLE then
		ffi.gc(hFile, ffi.C.FindClose)
		
		repeat
			fd.nFileSize.low, fd.nFileSize.high = fd.nFileSize.high, fd.nFileSize.low

			tFiles[ffi.string(fd.cFileName)] = {
				attrib = fd.dwFileAttributes,
				creationTime = fd.ftCreationTime,
				lastAccessTime = fd.ftLastAccessTime,
				lastWriteTime = fd.ftLastWriteTime,
				size = fd.nFileSize.packed,
				directory = (bit.band(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ~= 0)
			}
		until not ffi.C.FindNextFileA(hFile, fd)

		ffi.C.FindClose(ffi.gc(hFile, nil))
	end
	return tFiles
end

local files = getDirectoryFiles("data/shaders/", "*")
print(files)