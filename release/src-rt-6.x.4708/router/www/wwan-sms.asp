<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	WWAN SMS and Signal Strength by Michał Obrembski.

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: WWAN SMS</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram(''); %>	// http_id

var sms_remover = null;
var wwansms = '';
var wannum_selection = 1;
var wwansms_error;
var smsGrid = new TomatoGrid();

smsGrid.setup = function() {
	this.init('sms-grid', ['sort', 'delete']);
	this.headerSet(['ID', 'State', 'Date', 'Sender', 'Message']);
}
smsGrid.rpDel = function(e) {
	var smsToRemove = PR(e)._data[0];
	TomatoGrid.prototype.rpDel.call(this, e);
	removeSMS(smsToRemove);
}
smsGrid.populate = function() {
	/* Removing hasn't been done, wait until it finishes */
	if (sms_remover) return;
	var error_div = E('notice');
	if (wwansms_error) {
		error_div.style.display = 'inline-block';
		error_div.innerHTML = "<b>Error occurred!<\/b><br><br>Error message: " + wwansms_error;
	}
	else {
		error_div.style.display = 'none';
		var buf = wwansms.split('\n');
		var i;

		this.removeAllData();
		for (i = 0; i < buf.length; ++i) {
			var pduparseRegex = /^ID\:\s([0-9]+)\s\[(.*)\]\[(.*)\]\[(.*)\]\:\s(.*)$/g;
			var match = pduparseRegex.exec(buf[i]);
			if (match && match.length == 6) {
				this.insertData(-1, match.slice(1));
			}
		}
	}

	wwansms = '';
	spin(0);
}

function verifyFields(focused, quiet) {
	return true;
}

function showWait(x) {
	E('wait').style.display = (x ? 'block' : 'none');
	E('spin').style.display = (x ? 'inline-block' : 'none');
}

function removeSMS(smsNum) {
	if (sms_remover) return;
	showWait(1);
	sms_remover = new XmlHttp();
	sms_remover.onCompleted = function(text, xml) {
		showWait(0);
		sms_remover = null;
	}
	sms_remover.onError = function(x) {
		alert('error: ' + x);
		showWait(0);
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
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div id='sec-title' class="section-title">WWAN SMS list for modem </div>

<div id="notice" style="display:none"></div>

<div class="tomato-grid" id="sms-grid"></div>

<div id="wait">Please wait...&nbsp; <img src="spin.gif" alt="" id="spin"></div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,5,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
<script>smsGrid.setup()</script>
</body>
</html>
