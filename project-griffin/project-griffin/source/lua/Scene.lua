local ffi = require("ffi")

ffi.cdef[[
#include "source/api/SceneApi.h"
]]
local C = ffi.C
