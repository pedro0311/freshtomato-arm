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
<title>[<% ident(); %>] Tools: WOL</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% arplist(); %>

//	<% nvram('dhcpd_static,lan_ifname'); %>

var wg = new TomatoGrid();
wg.setup = function() {
	this.init('wol-grid', 'sort');
	this.headerSet(['MAC Address', 'IP Address', 'Status', 'Name']);
	this.sort(3);
}

wg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = 0;
	var c = this.sortColumn;
	if (c == 1)
		r = cmpIP(da[c], db[c]);
	else
		r = cmpText(da[c], db[c]);

	return this.sortAscending ? r : -r;
}

wg.populate = function() {
	var i, j, r, s;

	this.removeAllData();

	s = [];
	var q = nvram.dhcpd_static.split('>');
	for (i = 0; i < q.length; ++i) {
		var e = q[i].split('<');
		if (e.length == 4) {
			var m = e[0].split(',');
			for (j = 0; j < m.length; ++j) {
				s.push([m[j], e[1], e[2]]);
			}
		}
	}

	/* show entries in static dhcp list */
	for (i = 0; i < s.length; ++i) {
		var t = s[i];
		var active = '-';
		for (j = 0; j < arplist.length; ++j) {
			if ((arplist[j][2] == nvram.lan_ifname) && (t[0] == arplist[j][1])) {
				active = 'Active (In ARP)';
				arplist[j][1] = '!';
				break;
			}
		}
		if (t.length == 3) {
			r = this.insertData(-1, [t[0], (t[1].indexOf('.') != -1) ? t[1] : ('<% lanip(1); %>.' + t[1]), active, t[2]]);
			for (j = 0; j < 4; ++j)
				r.cells[j].title = 'Click to wake up';
		}
	}

	/* show anything else in ARP that is awake */
	for (i = 0; i < arplist.length; ++i) {
		if ((arplist[i][2] != nvram.lan_ifname) || (arplist[i][1].length != 17)) continue;
		r = this.insertData(-1, [arplist[i][1], arplist[i][0], 'Active (In ARP)', '']);
		for (j = 0; j < 4; ++j)
			r.cells[j].title = 'Click to wake up';
	}

	this.resort(2);
}

wg.onClick = function(cell) {
	wake(PR(cell).getRowData()[0]);
}

function verifyFields(focused, quiet) {
	var e;

	e = E('t_f_mac');
	e.value = e.value.replace(/[\t ]+/g, ' ');

	return (e.value ? 1 : 0);
}

function spin(x) {
	E('refresh-button').disabled = x;
	E('wakeb').disabled = x;
}

var waker = null;

function wake(mac) {
	if (!mac) {
		if (!verifyFields(null, 1)) return;
		mac = E('t_f_mac').value;
		cookie.set('wakemac', mac);
	}
	E('t_mac').value = mac;
	form.submit('t_fom', 1);
}

var refresher = null;
var timer = new TomatoTimer(refresh);
var running = 0;

function refresh() {
	if (!running) return;

	timer.stop();

	refresher = new XmlHttp();
	refresher.onCompleted = function(text, xml) {
		eval(text);
		wg.populate();
		timer.start(5000);
		refresher = null;
	}
	refresher.onError = function(ex) { alert(ex); reloadPage(); }
	refresher.post('update.cgi', 'exec=arplist');
}

function refreshClick() {
	running ^= 1;
	E('refresh-button').value = running ? 'Stop' : 'Refresh';
	E('refresh-spinner').style.display = (running ? 'inline-block' : 'none');
	if (running) refresh();
}

function init() {
	wg.recolor();
}
</script>
</head>

<body onload="init()">
<form id="t_fom" action="wakeup.cgi" method="post">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<input type="hidden" name="_redirect" value="tools-wol.asp">
<input type="hidden" name="_nextwait" value="1">
<input type="hidden" name="mac" value="" id="t_mac">

<!-- / / / -->

<div class="section-title">Wake On LAN</div>
<div class="section">
	<div class="tomato-grid" id="wol-grid"></div>

	<div><input type="button" value="Refresh" onclick="refreshClick()" id="refresh-button"> &nbsp; <img src="spin.gif" alt="" id="refresh-spinner"></div>
</div>

<!-- / / / -->

<div class="section-title"></div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'MAC Address List', name: 'f_mac', id: 't_f_mac', type: 'textarea', value: cookie.get('wakemac') || '' },
		]);
	</script>
	<div><input id="save-button" type="button" value="Wake Up" onclick="wake(null)"></div>
</div>

<!-- / / / -->

<div id="footer">
	&nbsp;
</div>

</td></tr>
</table>
</form>
<script>wg.setup();wg.populate();</script>
</body>
</html>
