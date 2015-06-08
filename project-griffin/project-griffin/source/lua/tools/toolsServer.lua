local ffi = require("ffi")

local copas = require("copas")
local socket = require("socket")

--local xavante = require("xavante.httpd")
--local hvhost = require("xavante.vhostshandler")
--local hurl = require("xavante.urlhandler")
local indexhandler = require("xavante.indexhandler")
local redirecthandler = require("xavante.redirecthandler")
local filehandler = require("xavante.filehandler")
--local cgiluahandler = require("xavante.cgiluahandler")

local xavante = require("xavante")

--local turbo = require("turbo")

ffi.cdef[[
#include "source/api/GriffinToolsApi.h"
]]
local C = ffi.C

function initToolsServer()
	local host = "*"
	local port = 8888

	-- Xavante http server
	-- Define here where Xavante HTTP documents scripts are located
	local webDir = "scripts/tools/web"

	local routes = {
		--[[{
			match = "^/$", --"^[^%./]*/$",
			with = indexhandler,
			params = { indexname = "/index.html" }
		},]]
		{ -- index
			match = "^[^%./]*/$",
			with = redirecthandler,
			params = { "index.html" }
		},
		--[[{ -- cgiluahandler example
			match = {"%.lp$", "%.lp/.*$", "%.lua$", "%.lua/.*$" },
			with = cgiluahandler.makeHandler(webDir)
		},]]
		{ -- filehandler
			match = ".",
			with = filehandler,
			params = { baseDir = webDir }
		}
	} 

	xavante.HTTP {
		server = { host = host, port = port },
		defaultHost = {
			rules = routes
		}
	}
end


function frameToolsHandler()
	if not copas.finished() then
		copas.step(0)
	end
end
