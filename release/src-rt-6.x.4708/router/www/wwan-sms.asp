<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	WWAN SMS and Signal Strength by Michał Obrembski.

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: WWAN SMS</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript" src="tomato.js"></script>

<!-- / / / -->

<style type="text/css">
#tp-grid .co1 {
	text-align: center;
	width: 30px;
}
#tp-grid .co3 {
	width: 80px;
}
#tp-grid .co2, #tp-grid .co4 {
	width: 120px;
}
#tp-grid .co5 {
	text-align: right;
	width: 440px;
}
#tp-grid .header .co1 {
	text-align: left;
}
</style>

<script type="text/javascript" src="debug.js"></script>

<script type="text/javascript">

//	<% nvram(''); %>	// http_id

var sms_remover = null;
var wwansms = '';
var wannum_selection = 1;
var wwansms_error;
var smsGrid = new TomatoGrid();

smsGrid.setup = function() {
	this.init('tp-grid', 'sort', ['delete']);
	this.headerSet(['ID', 'State', 'Date', 'Sender', 'Message']);
}
smsGrid.rpDel = function(e) {
	let smsToRemove = PR(e)._data[0];
	TomatoGrid.prototype.rpDel.call(this, e);
	removeSMS(smsToRemove);
}
smsGrid.populate = function() {
	/* Removing hasn't been done, wait until it finishes */
	if (sms_remover) return;
	let error_div = E('notice1');
	if (wwansms_error) {
		error_div.style.visibility = 'visible';
		error_div.innerHTML = "<b>Error occurred!<\/b><br><br>Error message: " + wwansms_error;
	} else {
		error_div.style.visibility = 'hidden';
		let buf = wwansms.split('\n');
		let i;

		this.removeAllData();
		for (i = 0; i < buf.length; ++i) {
			var pduparseRegex = /^ID\:\s([0-9]+)\s\[(.*)\]\[(.*)\]\[(.*)\]\:\s(.*)$/g;
			var match = pduparseRegex.exec(buf[i]);
			if (match && match.length == 6) {
				this.insertData(-1, match.slice(1));
			}
		}
	}

	E('debug').value = wwansms;
	wwansms = '';
	spin(0);
}

function verifyFields(focused, quiet) {
	return true;
}

function spin(x) {
	E('refresh-spinner').style.visibility = x ? 'visible' : 'hidden';
	if (!x) pinger = null;
}

function removeSMS(smsNum) {
	if (sms_remover) return;
	spin(1);
	sms_remover = new XmlHttp();
	sms_remover.onCompleted = function(text, xml) {
		spin(0);
		sms_remover = null;
	}
	sms_remover.onError = function(x) {
		alert('error: ' + x);
		spin(0);
		sms_remover = null;
	}

	sms_remover.post('wwansmsdelete.cgi', 'mwan_num=' + wannum_selection + '&sms_num=' + smsNum);
}

var ref;

function init() {
	if ((wannum_selection = cookie.get('wwansms_selection')) == null)
		wannum_selection = '1';
	E('sec-title').innerHTML = 'WWAN SMS list for WWAN modem ' + wannum_selection;
	ref = new TomatoRefresh('wwansms.cgi', 'mwan_num=' + wannum_selection, 0, 'wwan_sms_refresh');
	ref.refresh = function(text) {
		eval(text);
		smsGrid.populate();
	}
	ref.initPage(0, 5);
}
</script>

</head>
<body onload="init()">
<form action="javascript:{}">
<table id="container" cellspacing="0">
<tr><td colspan="2" id="header">
	<div class="title">Tomato</div>
	<div class="version">Version <% version(); %></div>
</td></tr>
<tr id="body"><td id="navi"><script type="text/javascript">navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div id='sec-title' class="section-title">WWAN SMS list for modem </div>

<div id="notice1" style="visibility:hidden;"></div>

<div id="tp-grid" class="tomato-grid"></div>
<pre id="stats"></pre>

<div style="height:10px;" onclick='E("debug").style.display=""'></div>
<textarea id="debug" style="width:99%;height:300px;display:none" cols="50" rows="10"></textarea>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">
	<script type="text/javascript">genStdRefresh(1,5,'ref.toggle()');</script>
</td></tr>
</table>
</form>
<script type="text/javascript">smsGrid.setup()</script>
</body>
</html>
