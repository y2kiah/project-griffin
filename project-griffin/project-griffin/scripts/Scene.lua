local ffi = require("ffi")

ffi.cdef[[

	//typedef void(*Callback_T)(griffin_FrameMappedInput*);

	// Functions

	
	uint64_t griffin_scene_createScene(const char name[32], bool makeActive);

	


]]
local C = ffi.C
