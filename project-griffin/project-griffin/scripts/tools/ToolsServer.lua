local ffi = require("ffi")
local copas = require("copas")
local socket = require("socket")

ffi.cdef[[


	





]]
local C = ffi.C;

--[[
local xavante = require("xavante.httpd")

local hvhost = require("xavante.vhostshandler")
local hurl = require("xavante.urlhandler")
local hindex = require("xavante.indexhandler")
local hfile = require("xavante.filehandler")
local hcgilua = require("xavante.cgiluahandler")


xavante.handle_request = hvhost({
	[""] = hurl({
		["/"] = hindex("/cgi/index.lp"),
		["/cgi/"] = hcgilua.makeHandler(XAVANTE_WEB),
		["/img/"] = hfile(XAVANTE_WEB.."/img"),
	})
})

xavante.register("*", 8080, "Griffin Tools 0.1.0")
]]


function initToolsServer()
	local host = "*"
	local port = 8080
	local server = socket.bind(host, port)

	function echoHandler(skt)
		copas.send(skt, "hello from griffin\n")
		while true do
			local data = copas.receive(skt)
			if data == "quit" then
				break
			end
			copas.send(skt, data)
		end
	end

	copas.addserver(server, echoHandler)
end


function frameToolsHandler()
	if not copas.finished() then
		copas.step(0)
	end
end
