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
<title>[<% ident(); %>] Logout...</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<style type="text/css">
#div {
	width:600px;
	height:180px;
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
input {
	width:80px;
	height:24px;
	cursor:pointer;
}
</style>
</head>
<body onload='setTimeout("go.submit()", 1200)'>
	<div id="div">
		<b>Logout</b>
		<br/>
		<hr style="height:1px">
		To clear the credentials cached by the browser:<br/>
		<br/>
		<b>Firefox, Internet Explorer, Opera, Safari</b><br/>
		- Leave the password field blank.<br/>
		- Click OK/Login<br/>
		<br/>
		<b>Chrome</b><br/>
		- Select Cancel.<br/><br/>
		<br/>
		<form action="logout" name="go" method="post">
			<div>
				<input type="hidden" value="<% nv(http_id); %>" name="_http_id">
			</div>
		</form>
	</div>
</body>
</html>