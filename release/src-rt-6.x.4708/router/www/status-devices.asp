<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Status: Device List</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="wireless.jsx?_http_id=<% nv(http_id); %>"></script>

<script>

//	<% nvram('lan_ifname,wl_ifname,wl_mode,wl_radio'); %>

//	<% devlist(); %>

ipp = '<% lipp(); %>.';

list = [];

function find(mac, ip) {
	var e, i;

	mac = mac.toUpperCase();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (((e.mac == mac) && ((e.ip == ip) || (e.ip == '') || (ip == null))) ||
			((e.mac == '00:00:00:00:00:00') && (e.ip == ip))) {
			return e;
		}
	}
	return null;
}

function get(mac, ip) {
	var e, i;

	mac = mac.toUpperCase();
	if ((e = find(mac, ip)) != null) {
		if (ip) e.ip = ip;
		return e;
	}

	e = {
		mac: mac,
		ip: ip || '',
		ifname: '',
		unit: 0,
		name: '',
		rssi: '',
		txrx: '',
		lease: ''
	};
	list.push(e);

	return e;
}


var xob = null;

function _deleteLease(ip, mac, wl) {
	form.submitHidden('dhcpd.cgi', {
		remove: ip,
		mac: mac,
		wl: wl });
}

function deleteLease(a, ip, mac, wl) {
	if (xob) return;
	if ((xob = new XmlHttp()) == null) {
		_deleteLease(ip, mac, wl);
		return;
	}

	a = E(a);
	a.innerHTML = 'deleting...';

	xob.onCompleted = function(text, xml) {
		a.innerHTML = '...';
		xob = null;
	}
	xob.onError = function() {
		_deleteLease(ip, mac, wl);
	}

	xob.post('dhcpd.cgi', 'remove=' + ip + '&mac=' + mac + '&wl=' + wl);
}

function addStatic(n) {
	var e = list[n];
	cookie.set('addstatic', [e.mac, e.ip, e.name.split(',')[0]].join(','), 1);
	location.href = 'basic-static.asp';
}

function addWF(n) {
	var e = list[n];
	cookie.set('addmac', [e.mac, e.name.split(',')[0]].join(','), 1);
	location.href = 'basic-wfilter.asp';
}

function addbwlimit(n) {
	var e = list[n];
	cookie.set('addbwlimit', [e.ip, e.name.split(',')[0]].join(','), 1);
	location.href = 'bwlimit.asp';
}

var ref = new TomatoRefresh('update.cgi', 'exec=devlist', 0, 'status_devices_refresh');

ref.refresh = function(text) {
	eval(text);
	dg.removeAllData();
	dg.populate();
	dg.resort();
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx)<0)
			E("noise"+uidx).innerHTML = wlnoise[uidx];
	}
}


var dg = new TomatoGrid();

dg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var ra = a.getRowData();
	var rb = b.getRowData();
	var r;

	switch (col) {
	case 2:
		r = cmpIP(ra.ip, rb.ip);
	break;
	case 4:
		r = cmpInt(ra.rssi, rb.rssi);
	break;
	case 5:
		r = cmpInt(ra.qual, rb.qual);
	break;
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}

	if (r == 0) {
		r = cmpIP(ra.ip, rb.ip);
		if (r == 0) r = cmpText(ra.ifname, rb.ifname);
	}

	return this.sortAscending ? r : -r;
}

dg.populate = function() {
	var i, j;
	var a, b, c, e;

	list = [];

	for (i = 0; i < list.length; ++i) {
		list[i].ip = '';
		list[i].ifname = '';
		list[i].unit = 0;
		list[i].name = '';
		list[i].rssi = '';
		list[i].txrx = '';
		list[i].lease = '';
	}

	for (i = wldev.length - 1; i >= 0; --i) {
		a = wldev[i];
		if (a[0].indexOf('wds') == 0) {
			e = get(a[1], '-');
		}
		else {
			e = get(a[1], null);
		}
		e.ifname = a[0];
		e.unit = a[6] * 1;
		e.rssi = a[2];

		if ((a[3] >= 1000) || (a[4] >= 1000))
		e.txrx = ((a[3] >= 1000) ? Math.round(a[3] / 1000) : '-') + ' / ' + ((a[4] >= 1000) ? Math.round(a[4] / 1000) : '-');
	}

	for (i = dhcpd_lease.length - 1; i >= 0; --i) {
		a = dhcpd_lease[i];
		e = get(a[2], a[1]);
		e.lease = '<small><a href="javascript:deleteLease(\'L' + i + '\',\'' + a[1] + '\',\'' + a[2] + '\',\'' + e.ifname + '\')" title="Delete Lease' + (e.ifname ? " and Deauth" : "") + '" id="L' + i + '">' + a[3] + '<\/a><\/small>';
		e.name = a[0];
	}

	for (i = arplist.length - 1; i >= 0; --i) {
		a = arplist[i];

		if ((e = get(a[1], a[0])) != null) {
			if (e.ifname == '') e.ifname = a[2];
		}
	}

	for (i = dhcpd_static.length - 1; i >= 0; --i) {
		a = dhcpd_static[i].split('<');
		if (a.length < 3) continue;

		if (a[1].indexOf('.') == -1) a[1] = (ipp + a[1]);

		c = a[0].split(',');
		for (j = c.length - 1; j >= 0; --j) {
			if ((e = find(c[j], a[1])) != null) break;
		}
		if (j < 0) continue;

		if (e.ip == '') {
			e.ip = a[1];
		}

		if (e.name == '') {
			e.name = a[2];
		}
		else {
			b = e.name.toLowerCase();
			c = a[2].toLowerCase();
			if ((b.indexOf(c) == -1) && (c.indexOf(b) == -1)) {
				if (e.name != '') e.name += ', ';
				e.name += a[2];
			}
		}
	}

	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];

		b = e.mac;
		if (e.mac.match(/^(..):(..):(..)/)) {
			b += '<br><small class="pics">' +
				'<a href="http://api.macvendors.com/' + RegExp.$1 + '-' + RegExp.$2 + '-' + RegExp.$3 + '" class="new_window" title="OUI Search">[oui]<\/a> ' +
				'<a href="javascript:addStatic(' + i + ')" title="Static Lease">[static]<\/a> ' +
				'<a href="javascript:addbwlimit(' + i + ')" title="BW Limiter">[bwlimit]<\/a>';

			if (e.rssi != '') {
				b += ' <a href="javascript:addWF(' + i + ')" title="Wireless Filter">[wfilter]<\/a>';
			}
			b += '<\/small>';
		}
		else {
			b = '';
		}

		var ifidx = wl_uidx(e.unit);
		if ((e.rssi !== '') && (ifidx >= 0) && (wlnoise[ifidx] < 0)) {
			if (e.rssi >= -50) {
				e.qual = 100;
			}
			else if (e.rssi >= -80) { /* between -50 ~ -80dbm */
				e.qual = Math.round(24 + ((e.rssi + 80) * 26)/10);
			}
			else if (e.rssi >= -90) { /* between -80 ~ -90dbm */
				e.qual = Math.round(24 + ((e.rssi + 90) * 26)/10);
			}
			else {
				e.qual = 0;
			}
		}
		else {
			e.qual = -1;
		}

		this.insert(-1, e, [
			e.ifname, b, (e.ip == '-') ? '' : e.ip, e.name,
			(e.rssi != 0) ? e.rssi + ' <small>dBm<\/small>' : '',
			(e.qual < 0) ? '' : '<small>' + e.qual + '<\/small> <img src="bar' + MIN(MAX(Math.floor(e.qual / 12), 1), 6) + '.gif" alt="">',
			e.txrx, e.lease], false);
	}
}

dg.setup = function() {
	this.init('dev-grid', 'sort');
	this.headerSet(['Interface', 'MAC Address', 'IP Address', 'Name', 'RSSI &nbsp; &nbsp; ', 'Quality', 'TX/RX Rate&nbsp;', 'Lease &nbsp; &nbsp; ']);
	this.populate();
	this.sort(2);
}

var observer = window.MutationObserver || window.WebKitMutationObserver || window.MozMutationObserver;

function earlyInit() {
	if (observer)
		new observer(eventHandler).observe(E("dev-grid"), { childList: true, subtree: true });
	dg.setup();
}

function init() {
	dg.recolor();
	ref.initPage(3000, 3);
}
</script>
</head>

<body onload="init()">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div class="section-title">Device List</div>
<div class="section">
	<div class="tomato-grid" id="dev-grid"></div>

	<script>
		f = [];
		for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
			var u = wl_unit(uidx);
			if (nvram['wl'+u+'_radio'] == '1') {
				if (wl_sunit(uidx) < 0) {
					f.push( { title: '<b>Noise Floor (' + wl_ifaces[uidx][0] + ')&nbsp;:<\/b>', prefix: '<span id="noise'+uidx+'">', custom: wlnoise[uidx], suffix: '<\/span>&nbsp;<small>dBm<\/small>' } );
				}
			}
		}
		createFieldTable('', f);
	</script>
</div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,0,'ref.toggle()');</script>
</div>

</td></tr>
</table>
<script>earlyInit();</script>
</body>
</html>
