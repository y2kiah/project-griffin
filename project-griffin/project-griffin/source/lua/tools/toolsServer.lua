
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