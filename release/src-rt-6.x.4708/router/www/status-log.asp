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
<title>[<% ident(); %>] Status: Logs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("log_file"); %>

function find() {
	var s = E('find-text').value;
	if (s.length) document.location = 'logs/view.cgi?find=' + escapeCGI(s) + '&_http_id=' + nvram.http_id;
}

function init() {
	if (nvram.log_file != '1') {
		E('logging').style.display = 'none';
		E('note-disabled').style.display = 'block';
		return;
	}

	var e = E('find-text');
	if (e) e.onkeypress = function(ev) {
		if (checkEvent(ev).keyCode == 13) find();
	}
}
</script>
</head>

<body onload="init()">
<form id="t_fom" action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div class="section-title">Logs</div>
<div id="logging">
	<div class="section">
		<a href="logs/view.cgi?which=25&amp;_http_id=<% nv(http_id) %>">View Last 25 Lines</a><br>
		<a href="logs/view.cgi?which=50&amp;_http_id=<% nv(http_id) %>">View Last 50 Lines</a><br>
		<a href="logs/view.cgi?which=100&amp;_http_id=<% nv(http_id) %>">View Last 100 Lines</a><br>
		<a href="logs/view.cgi?which=all&amp;_http_id=<% nv(http_id) %>">View All</a><br><br>
		<a href="logs/syslog.txt?_http_id=<% nv(http_id) %>">Download Log File</a><br><br>
		<input type="text" maxlength="32" size="33" id="find-text"> <input type="button" value="Find" onclick="find()"><br>
		<br><br>
		&raquo; <a href="admin-log.asp">Logging Configuration</a><br><br>
	</div>
</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>Internal logging disabled.</b><br><br><a href="admin-log.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
</body>
</html>
