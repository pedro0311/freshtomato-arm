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
<title>[<% ident(); %>] Measuring Noise...</title>
<script type="text/javascript">
function tick() {
	t.innerHTML = tock;
	if (--tock >= 0)
		setTimeout(tick, 1000);
	else
		history.go(-1);
}

function init() {
	t = document.getElementById('sptime');
	tock = 15;
	tick();
}
</script>
<style type="text/css">
body {
	background:rgb(0,0,0) url(tomatousb_bg.png);
	font:14px Tahoma,Arial,sans-serif;
	color:rgb(255,255,255);
}
input {
	width:80px;
	height:24px;
}
.div {
	width:600px;
	height:75px;
	background-color:rgb(47,61,64);
	position:absolute;
	top:0;
	bottom:0;
	left:0;
	right:0;
	text-align:center;
	margin:auto;
	padding:10px 10px;
	border-radius:5px;
}
</style>
</head>

<body onload="init()" onclick="go()">
	<div class="div">
		<div style="font-size:25px">Measuring radio noise floor...</div>
		<br/>
		<div>Wireless access has been temporarily disabled for <div id="sptime" style="display:inline;background:rgb(110,10,10);padding:2px 2px;border-radius:2px"></div> second(s)</div>
	</div>
</body>
</html>