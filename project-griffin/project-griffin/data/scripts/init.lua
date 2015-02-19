local ffi = require ("ffi")

ffi.cdef[[
void debug_printf(const char *fmt);
]]

ffi.C.debug_printf("Hello World from Lua" .. "JIT" .. " script!")