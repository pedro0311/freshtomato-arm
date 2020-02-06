<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/
For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Shutting down...</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript">
var n = 31;
function tick() {
	if (--n > 0) {
		document.getElementById("sptime").innerHTML = n;
		setTimeout(tick, 1000);
	} else {
		document.getElementById("msg").innerHTML = "You can now unplug the router.";
	}
}
</script>
<style type="text/css">
#div {
	width:400px;
	height:40px;
	color:rgb(200,200,200);
	background-color:rgb(22,22,22);
	position:absolute;
	top:0;
	bottom:0;
	left:0;
	right:0;
	font:14px Tahoma,Arial,sans-serif;
	text-align:center;
	margin:auto;
	padding:10px 10px;
	border-radius:5px;
}
#sptime {
	display:inline;
	background:rgb(140,10,10);
	padding:2px 2px;
	border-radius:2px;
}
input {
	width:80px;
	height:24px;
}
</style>
</head>
<body onload="tick()">
	<div id="div">
		<div style="padding:10px 0">
			<div id="msg">Please wait while the router shuts down... &nbsp;<div id="sptime"></div></div>
		</div>
	</div>
</body>
</html>