<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Measuring Noise...</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<style>
div.tomato-grid.container-div {
	height: 75px;
}
</style>
<script>
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
</head>

<body onload="init()">
<div class="tomato-grid container-div">
	<div class="wrapper1">
		<div class="wrapper2">
			<div class="info-centered">
				<div style="font-size:20px">Measuring radio noise floor...</div>
				<br>
				Wireless access has been temporarily disabled for <div id="sptime"></div> second(s)
			</div>
		</div>
	</div>
</div>
</body>
</html>
