JSON = require("JSON")
JSON.strictTypes = true

cgilua.contentheader("application", "json")
cgilua.put(JSON:encode({ hi="hi", bye="bye", method=cgilua.QUERY.method or "" }))