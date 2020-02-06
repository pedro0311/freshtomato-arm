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
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
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
#div {
	width:600px;
	height:75px;
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
</style>
</head>

<body onload="init()" onclick="go()">
	<div id="div">
		<div style="font-size:20px">Measuring radio noise floor...</div>
		<br/>
		<div>Wireless access has been temporarily disabled for <div id="sptime"></div> second(s)</div>
	</div>
</body>
</html>