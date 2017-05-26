<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
	Tomato GUI
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] NAS: UPS Monitor</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>
<style type='text/css'>
textarea {
	width: 98%;
	height: 5em;
}
#dev-grid .co1 {
	width: 10%;
}
#dev-grid .co2 {
	width: 9%;
}
#dev-grid .co3 {
	width: 65%;
}
#dev-grid .co4 {
	width: 16%;
	text-align: center;
}
#dev-grid .header {
	text-align: left;
}
</style>
<script type='text/javascript'>

//	<% nvram("usb_apcupsd"); %>

function init()
{
	clientSideInclude('ups-status', '/ext/cgi-bin/tomatoups.cgi');
	clientSideInclude('ups-data', '/ext/cgi-bin/tomatodata.cgi');
}

function clientSideInclude(id, url) {
	var req = false;
	// For Safari, Firefox, and other non-MS browsers
	if (window.XMLHttpRequest) {
		try {
			req = new XMLHttpRequest();
		} catch (e) {
			req = false;
		}
	} else if (window.ActiveXObject) {
		// For Internet Explorer on Windows
		try {
			req = new ActiveXObject("Msxml2.XMLHTTP");
		} catch (e) {
			try {
				req = new ActiveXObject("Microsoft.XMLHTTP");
			} catch (e) {
				req = false;
			}
		}
	}
	var element = document.getElementById(id);

	if (!element) {
		alert("Bad id " + id + "passed to clientSideInclude. You need a div or span element with this id in your page.");
	return;
	}

	if (req) {
		// Synchronous request, wait till we have it all
		req.open('GET', url, false);
		req.send(null);
		element.innerHTML = req.responseText;
	} else {
		element.innerHTML = "Sorry, your browser does not support XMLHTTPRequest objects. This page requires " +
							"Internet Explorer 5 or better for Windows, or Firefox for any system, or Safari. Other " +
							"compatible browsers may also exist.";
	}
}
</script>
</head>
<body onload='init()'>
<form id='t_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi' style="height: 188px"><script type='text/javascript'>navi()</script></td>
<td id='content' style="height: 188px">
	<div id='ident'><% ident(); %></div>


	<div class='section-title' id="upsstatus" style="display:none">UPS Monitor</div>
	<div id="upsmonitor">

		<input type='hidden' name='_nextpage' value='nas-ups.asp'>
		<div class='section-title' id="ups-status-section">APC UPS Status</div>
		<div class='section'>
			<span id="ups-status"></span>
		</div>
		<div class='section-title' id="ups-data-section">APC UPS Response</div>
		<div class='section'>
			<span id="ups-data"></span>
		</div>

	<br />

	</div>

<!-- / / / -->

<script type='text/javascript'>
	if (nvram.usb_apcupsd != '1') {
		W('<div class="note-disabled"><b>UPS Monitor disabled.<\/b><br /><br /><a href="nas-usb.asp">Enable &raquo;<\/a><\/div>');
		E('upsmonitor').style.display = 'none';
		E('upsstatus').style.display = 'block';
	}
</script>

<!-- / / / -->


</td></tr>
<tr><td id='footer' colspan=2>&nbsp;</td></tr>
</table>
</form>
</body>
</html>