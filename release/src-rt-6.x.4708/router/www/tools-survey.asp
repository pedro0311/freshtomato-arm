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
<title>[<% ident(); %>] Tools: Wireless Survey</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram(''); %>	// http_id

var wlscandata = [];
var entries = [];
var dayOfWeek = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

Date.prototype.toWHMS = function() {
	return dayOfWeek[this.getDay()] + ' ' + this.getHours() + ':' + this.getMinutes().pad(2)+ ':' + this.getSeconds().pad(2);
}

var sg = new TomatoGrid();

sg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 0:
		r = -cmpDate(da.lastSeen, db.lastSeen);
	break;
	case 3:
		r = cmpInt(da.rssi, db.rssi);
	break;
	case 4:
		r = cmpInt(da.qual, db.qual);
	break;
	case 5:
		r = cmpInt(da.channel, db.channel);
	break;
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}

	if (r == 0) r = cmpText(da.bssid, db.bssid);

	return this.sortAscending ? r : -r;
}

sg.rateSorter = function(a, b) {
	if (a < b) return -1;
	if (a > b) return 1;

	return 0;
}

sg.populate = function() {
	var added = 0;
	var removed = 0;
	var i, j, k, t, e, s;

	if ((wlscandata.length == 1) && (!wlscandata[0][0])) {
		setMsg("error: " + wlscandata[0][1]);
		return;
	}

	for (i = 0; i < wlscandata.length; ++i) {
		s = wlscandata[i];
		e = null;

		for (j = 0; j < entries.length; ++j) {
			if (entries[j].bssid == s[0]) {
				e = entries[j];
				break;
			}
		}
		if (!e) {
			++added;
			e = {};
			e.firstSeen = new Date();
			entries.push(e);
		}
		e.lastSeen = new Date();
		e.bssid = s[0];
		e.ssid = s[1];
		e.channel = s[3];
		e.channel = e.channel + '<br><small>' + s[9] + ' GHz<\/small>'+ '<br><small>' + s[4] + ' MHz<\/small>';
		e.rssi = s[2];
		e.cap = s[7]+ '<br>' +s[8];
		e.rates = s[6];
		e.qual = Math.round(s[5]);
	}

	t = E('expire-time').value;
	if (t > 0) {
		var cut = (new Date()).getTime() - (t * 1000);
		for (i = 0; i < entries.length; ) {
			if (entries[i].lastSeen.getTime() < cut) {
				entries.splice(i, 1);
				++removed;
			}
			else
				++i;
		}
	}

	for (i = 0; i < entries.length; ++i) {
		var seen, m, mac;

		e = entries[i];

		seen = e.lastSeen.toWHMS();
		if (useAjax()) {
			m = Math.floor(((new Date()).getTime() - e.firstSeen.getTime()) / 60000);
			if (m <= 10) seen += '<br> <b><small>NEW (' + m + 'm)<\/small><\/b>';
		}

		mac = e.bssid;
		if (mac.match(/^(..):(..):(..)/))
			mac = '<a href="http://api.macvendors.com/' + RegExp.$1 + '-' + RegExp.$2 + '-' + RegExp.$3 + '" class="new_window" title="OUI search">' + mac + '<\/a>';

		sg.insert(-1, e, [
			'<small>' + seen + '<\/small>',
			'' + e.ssid,
			mac,
			(e.rssi == -999) ? '' : (e.rssi + ' <small>dBm<\/small>'),
			'<small>' + e.qual + '<\/small> <img src="bar' + MIN(MAX(Math.floor(e.qual / 10), 1), 6) + '.gif" alt="">',
			'' + e.channel,
			'' + e.cap,
			'' + e.rates], false);
	}

	s = '';
	if (useAjax()) s = added + ' added, ' + removed + ' removed, ';
	s += entries.length + ' total.';

	s += '<br><br><small>Last updated: ' + (new Date()).toWHMS() + '<\/small>';
	setMsg(s);

	wlscandata = [];
}

sg.setup = function() {
	this.init('survey-grid', 'sort');
	this.headerSet(['Last Seen', 'SSID', 'BSSID', 'RSSI &nbsp; &nbsp; ', 'Quality', 'Control Channel', 'Security', 'Rates']);
	this.populate();
	this.sort(0);
}

function setMsg(msg) {
	E('survey-msg').innerHTML = msg;
}

var ref = new TomatoRefresh('update.cgi', 'exec=wlscan', 0, 'tools_survey_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
		return;
	}
	sg.removeAllData();
	sg.populate();
	sg.resort();
}

var observer = window.MutationObserver || window.WebKitMutationObserver || window.MozMutationObserver;

function init() {
	if (observer)
		new observer(eventHandler).observe(E("survey-grid"), { childList: true, subtree: true });
	sg.recolor();
	ref.initPage();
}

function earlyInit() {
	if (!useAjax()) E('expire-time').style.display = 'none';
	sg.setup();
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

<div class="section-title">Wireless Site Survey</div>
<div class="section">
	<div id="survey-grid" class="tomato-grid"></div>
	<div id="survey-msg"></div>

	<div id="survey-warn">
		<script>
			if ('<% wlclient(); %>' == '0') {
				document.write('<b>Warning:<\/b> Wireless connections to this router may be disrupted while using this tool.');
			}
		</script>
	</div>
</div>

<!-- / / / -->

<div id="footer">
	<div id="survey-controls">
		<img src="spin.gif" alt="" id="refresh-spinner">
		<script>
			genStdTimeList('expire-time', 'Expire After', 0);
			genStdTimeList('refresh-time', 'Refresh Every', 0);
		</script>
		<input type="button" value="Refresh" onclick="ref.toggle()" id="refresh-button">
	</div>
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
