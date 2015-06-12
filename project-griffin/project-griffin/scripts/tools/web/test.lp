<%
local lfs = require "lfs"
%>
<!doctype html>
<html>
<head>
	<meta charset="utf-8">
	<title>GriffinTools-0.1.0</title>
	<link rel="shortcut icon" href="favicon.png"> 
</head>
<body>
    <p><%= lfs.currentdir() %></p>
</body>
</html>