package.path  = "./scripts/?.lua;./scripts/extension/?.lua;../../../scripts/?.lua;../../../scripts/extension/?.lua;./?.lua"
package.cpath = "./scripts/?.dll;./scripts/?.so;./scripts/extension/?.dll;./scripts/extension/?.so;../../../scripts/?.dll;../../../scripts/?.so;../../../scripts/extension/?.dll;../../../scripts/extension/?.so"

local ffi = require("ffi")

local copas = require("copas")
local socket = require("socket")

local xavante = require("xavante")
local indexhandler = require("xavante.indexhandler")
local redirecthandler = require("xavante.redirecthandler")
local filehandler = require("xavante.filehandler")
local cgiluahandler = require("xavante.cgiluahandler")
local orbithandler = require("orbit.ophandler")
local wsx = require("wsapi.xavante")
local sapi = require("wsapi.sapi")


ffi.cdef[[


	





]]
local C = ffi.C

function initToolsServer()
	local host = "*"
	local port = 8888

	-- Xavante http server
	-- Define here where Xavante HTTP documents scripts are located
	local webDir = "scripts/tools/web"

	local rules = {
--		{
--			match = "^/$", --"^[^%./]*/$",
--			with = indexhandler,
--			params = { indexname = "/index.html" }
--		},
		{ -- cgiluahandler example
			match = { "%.lp$", "%.lp/.*$", "%.lua$", "%.lua/.*$" },
			with = cgiluahandler.makeHandler(webDir)
		},
		{ -- wsapihandler example
			match = { "%.ws$", "%.ws/" },
			with = wsx.makeGenericHandler(webDir)
		},
--		{ -- orbitpages handler
--			match = { "%.op$", "%.op/.*$" },
--			with = orbithandler.makeHandler(webDir)
--		},
		{ -- index redirect
			match = "^[^%./]*/$",
			with = redirecthandler,
			params = { "index.html" }
		},
		{ -- filehandler
			match = ".",
			with = filehandler,
			params = { baseDir = webDir }
		}
	} 

	xavante.HTTP {
		server = { host = host, port = port },
		defaultHost = {
			rules = rules
		}
	}
end


function frameToolsHandler()
	if not copas.finished() then
		copas.step(0)
	end
end
