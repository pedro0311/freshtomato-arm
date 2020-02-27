<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/
For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Please Wait...</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript">
var spun = 1;
wait = parseInt('<% cgi_get("_nextwait"); %>', 10);
if (isNaN(wait)) wait = 5;
function tick() {
	clock.innerHTML = wait;
	opacity -= step;
	if (opacity < 0) opacity = 0;
	spin.style.opacity = opacity.toFixed(4);
	if (--wait >= 0) setTimeout(tick, 1000);
	else go();
}
function go() {
	clock.style.visibility = "hidden";
	window.location.replace('<% cgi_get("_nextpage"); %>');
}
function setSpin(x) {
	l2.style.display = x ? "block" : "none";
	spun = x;
}
function init() {
	if (wait > 0) {
		spin = document.getElementById("spin");
		opacity = 1;
		step = 1 / wait;
		l2 = document.getElementById("l2");
		l2.style.display = "block";
		clock = document.getElementById('xclock');
		clock.style.visibility = 'visible';
		tick();
		if (!spun) setSpin(0);  /* http may be down after this page gets sent */
	} else {
		l1 = document.getElementById("l1");
		l1.style.display = "block";
	}
}
</script>
<style type="text/css">
#div {
	width:600px;
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
#xclock {
	background:rgb(140,10,10);
	padding:2px 2px;
	border-radius:2px;
	visibility:hidden;
}
#spin {
	vertical-align:bottom;
}
input {
	width:80px;
	height:24px;
}
</style>
</head>
<body onload="init()" onclick="go()">
	<div id="div">
		<form action="">
			<div style="padding:10px 0">
				<div id="l1" style="display:none"><b>Changes Saved... </b> <input type="button" value="Continue" onclick="go()"></div>
				<div id="l2" style="display:none"><b>Please Wait... </b> &nbsp;<span id="xclock"></span> &nbsp;<img src="spin.gif" id="spin" alt=""></div>
			</div>
		</form>
	</div>
</body>
</html>