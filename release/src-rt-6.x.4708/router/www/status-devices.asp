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

//	<% devlist(); %>

var lipp = '<% lanip(1); %>.';

var list = [];
var list_last = [];
var xob = null;
var cmd = null;
var cmdresult = '';

var ref = new TomatoRefresh('update.cgi', 'exec=devlist', 5, 'status_devices_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
	}

	dg.removeAllData();
	dg.populate();
	dg.resort();
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx) < 0)
			E('noise'+uidx).innerHTML = wlnoise[uidx];
	}
}

function find(mac, ip) {
	var e, i;

	mac = mac.toUpperCase();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (((e.mac == mac) && ((e.ip == ip) || (e.ip == '') || (ip == null))) || ((e.mac == mac_null) && (e.ip == ip)))
			return e;
	}

	return null;
}

function get(mac, ip) {
	var e, i;

	mac = mac.toUpperCase();
	if ((e = find(mac, ip)) != null) {
		if (ip)
			e.ip = ip;

		return e;
	}

	e = {
		mac: mac,
		ip: ip || '',
		ifname: '',
		ifstatus: '',
		bridge: '',
		freq: '',
		ssid: '',
		mode: '',
		unit: 0,
		name: '',
		rssi: '',
		txrx: '',
		lease: '',
		lan: '',
		wan: '',
		proto: '',
		media: ''
	};
	list.push(e);

	return e;
}
function spin(x, which) {
	E(which).style.display = (x ? 'inline-block' : 'none');
	if (!x)
		cmd = null;
}

function _deleteLease(ip, mac, wl) {
	form.submitHidden('dhcpd.cgi', { remove: ip, mac: mac, wl: wl });
}

function deleteLease(a, ip, mac, wl) {
	if (xob)
		return;

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

	xob.post('dhcpd.cgi', 'remove='+ip+'&mac='+mac+'&wl='+wl);
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

function addRestrict(n) {
	var e = list[n];
	cookie.set('addrestrict', [e.mac, e.name.split(',')[0]].join(','), 1);
	form.submitHidden('tomato.cgi', { _redirect: 'restrict-edit.asp', rruleN: -1 });
}

function spin(x, which) {
	E(which).style.display = (x ? 'inline-block' : 'none');
	if (!x)
		cmd = null;
}

function displayOUI(i) {
	spin(0, 'gW_'+i);
	if (cmdresult.indexOf('Not Found') == -1)
		cmdresult = 'Manufacturer: \n'+cmdresult;
	else
		cmdresult = 'Manufacturer not found!';

	alert(cmdresult);
	cmdresult = '';
}

function searchOUI(n, i) {
	spin(1, 'gW_'+i);

	cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		eval(text);
		displayOUI(i);
	}
	cmd.onError = function(x) {
		cmdresult = 'ERROR: '+x;
		displayOUI(i);
	}

	var commands = '/usr/bin/wget -T 6 -q http://api.macvendors.com/'+n+' -O /tmp/oui.txt \n /bin/cat /tmp/oui.txt';
	cmd.post('shell.cgi', 'action=execute&command='+escapeCGI(commands.replace(/\r/g, '')));
}

var dg = new TomatoGrid();

dg.setup = function() {
	this.init('dev-grid', 'sort');
	this.headerSet(['Interface', 'Media', 'MAC Address', 'IP Address', 'Name', 'RSSI', 'Quality &nbsp;', 'TX/RX<br>Rate', 'Lease&nbsp;']);
	this.populate();
	this.sort(3);
}

dg.populate = function() {
	var i, j, k, l;
	var a, b, c, e, f;
	var mode = '', wan_gw;

/* IPV6-BEGIN */
	var i2, e2;
/* IPV6-END */

	list = [];
	wl_info = [];

	for (i = 0; i < list.length; ++i) {
		list[i].ip = '';
		list[i].ifname = '';
		list[i].ifstatus = '';
		list[i].bridge = '';
		list[i].freq = '';
		list[i].ssid = '';
		list[i].mode = '';
		list[i].unit = 0;
		list[i].name = '';
		list[i].rssi = '';
		list[i].txrx = '';
		list[i].lease = '';
		list[i].lan = '';
		list[i].wan = '';
		list[i].proto = '';
		list[i].media = '';
	}

	/* [ "eth1", "0", 0, -1, "SSID", "MAC", 1, 16, "wet", "MAC2" ] */
	for (i = 0; i < wl_ifaces.length; ++i) {
		var j = wl_fface(i);
		a = wl_ifaces[i];
		c = wl_display_ifname(i);
		if (a[6] != 1)
			b = 'Down';
		else {
			for (j = 0; j < xifs[0].length ; ++j) {
				if ((nvram[xifs[0][j]+'_ifnames']).indexOf(a[0]) >= 0) {
					b = xifs[1][j];
					break;
				}
			}
		}
		wl_info.push([a[0], c.substr(c.indexOf('/') + 2), a[4], a[8], b]);
	}

	/*  [ "wl0.1/eth1/2/3", "MAC", -53, 39000, 144444, 56992, (unit[0/1/2]) ] */
	for (i = wldev.length - 1; i >= 0; --i) {
		a = wldev[i];
		if (a[0].indexOf('wds') == 0)
			e = get(a[1], '-');
		else
			e = get(a[1], null);

		e.ifname = a[0];
		e.unit = a[6] * 1;
		e.rssi = a[2];

		for (j = 0; j < wl_info.length; ++j) {
			if (wl_info[j][0] == e.ifname) {
				e.freq = wl_info[j][1];
				e.ssid = wl_info[j][2];
				e.mode = wl_info[j][3];
				e.ifstatus = wl_info[j][4];
				if (e.mode == 'wet')
					mode = e.mode;
			}
		}

		if ((a[3] >= 1000) || (a[4] >= 1000))
			e.txrx = ((a[3] >= 1000) ? Math.round(a[3] / 1000) : '-')+' / '+((a[4] >= 1000) ? Math.round(a[4] / 1000) : '-');
	}

	/* special case: pppoe/pptp/l2tp WAN */
	for (i = 1; i <= MAX_PORT_ID; i++) {
		k = (i == 1) ? '' : i.toString();
		var proto = nvram['wan'+k+'_proto'];
		if ((proto == 'pppoe' || proto == 'pptp' || proto == 'l2tp') && nvram['wan'+k+'_hwaddr']) {
			e = get(nvram['wan'+k+'_hwaddr'], null);
			e.ifname = (nvram['wan'+k+'_iface'] ? nvram['wan'+k+'_iface'] : (nvram['wan'+k+'_ifname'] ? nvram['wan'+k+'_ifname'] : 'ppp+'));
			var face = (nvram['wan'+k+'_ifname'] ? nvram['wan'+k+'_ifname'] : (nvram['wan'+k+'_ifnameX'] ? nvram['wan'+k+'_ifnameX'] : ''));
			var ip = nvram['wan'+k+'_ppp_get_ip'];
			var gw = nvram['wan'+k+'_gateway_get'];
			if ((!gw) || (gw == '0.0.0.0'))
				gw = nvram['wan'+k+'_gateway'];

			e.ip = 'r:&nbsp;'+gw+(ip && ip != '0.0.0.0' ? '<br>l:&nbsp;'+ip : '');
			var ip2 = nvram['wan'+k+'_ipaddr'];
			var gw2 = nvram['wan'+k+'_gateway'];
			if (nvram['wan'+k+'_pptp_dhcp'] == '1') {
				if (gw2 && gw2 != '0.0.0.0' && gw2 != gw && ip2 && ip2 != '0.0.0.0' && ip2 != ip) {
					e.ip = 'r:&nbsp;'+gw+'<br>l:&nbsp;'+ip;
					e.name = 'r:&nbsp;'+gw2+(face ? '&nbsp;<small>('+face+')<\/small>' : '')+'<br>l:&nbsp;'+ip2+(face ? '&nbsp;<small>('+face+')<\/small>' : '');
				}
			}
			else {
				if (proto == 'pptp') /* is this correct? feedback needed */
					e.ip = 'r:&nbsp;'+nvram['wan'+k+'_pptp_server_ip']+'<br>l:&nbsp;'+ip;
			}
		}
	}

	/* [ "name", "IP", "MAC", "0 days, 23:46:28" ] */
	for (i = dhcpd_lease.length - 1; i >= 0; --i) {
		a = dhcpd_lease[i];
		e = get(a[2], a[1]);
		b = a[3].indexOf(',') + 1;
		c = a[3].slice(0, b)+'<br>'+a[3].slice(b);
		e.lease = '<small><a href="javascript:deleteLease(\'L'+i+'\',\''+a[1]+'\',\''+a[2]+'\',\''+e.ifname+'\')" title="Delete Lease'+(e.ifname ? ' and Deauth' : '')+'" id="L'+i+'">'+c+'<\/a><\/small>';
		e.name = a[0];
	}

	/* [ "IP", "MAC", "br0/wwan0", "name" ] */
	for (i = arplist.length - 1; i >= 0; --i) {
		a = arplist[i];
		if ((e = get(a[1], a[0])) != null) {
			if (e.ifname == '')
				e.ifname = a[2];

			e.bridge = a[2];
			if (e.name == '')
				e.name = a[3];
		}
	}

	/* [ "MAC", "IP", "name", "BoundTo" ] */
	var dhcpd_static = nvram.dhcpd_static.split('>');
	for (i = dhcpd_static.length - 1; i >= 0; --i) {
		a = dhcpd_static[i].split('<');
		if (a.length < 4)
			continue;

		/* find and compare MAC(s) */
		c = a[0].split(',');

		loop1:
		for (j = c.length - 1; j >= 0; --j) {
			if ((e = find(c[j], a[1])) == null) {
				/* special case for gateway */
				for (l = 1; l <= MAX_PORT_ID; l++) {
					k = (l == 1) ? '' : l.toString();
					wan_gw = nvram['wan'+k+'_gateway'];
					if (wan_gw != '' && wan_gw != '0.0.0.0' && (e = find(c[j], null)) != null && e.ip.substr(0, e.ip.lastIndexOf('.') + 1) != lipp) { /* FIXME: loop needed for every active bridge */
						e.ip = nvram['wan'+k+'_gateway'];
						break loop1;
					}
				}
			}
			else
				break;
		}
		if (j < 0)
			continue;

		if (e.ip == '')
			e.ip = a[1];

		/* empty name - add */
		if (e.name == '')
			e.name = a[2];
		/* not empty: compare name from dhcpd_lease and dhcpd_static, if different - add */
		else {
			b = e.name.toLowerCase();
			c = a[2].toLowerCase();
			if ((b.indexOf(c) == -1) && (c.indexOf(b) == -1))
				e.name += ', '+a[2];
		}
	}

	/* step 1: prepare list */
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];

		var ifidx = wl_uidx(e.unit);
		if ((e.rssi !== '') && (ifidx >= 0) && (wlnoise[ifidx] < 0)) {
			if (e.rssi >= -50)
				e.qual = 100;
			else if (e.rssi >= -80) /* between -50 ~ -80dbm */
				e.qual = Math.round(24 + ((e.rssi + 80) * 26)/10);
			else if (e.rssi >= -90) /* between -80 ~ -90dbm */
				e.qual = Math.round(24 + ((e.rssi + 90) * 26)/10);
			else
				e.qual = 0;
		}
		else
			e.qual = -1;

		/* fix problem with arplist */
		if (e.bridge == '' && e.ip != '-') {
			for (j = 0; j <= MAX_BRIDGE_ID; j++) {
				k = (j == 0) ? '' : j.toString();
				if (nvram['lan'+k+'_ipaddr'] && (nvram['lan'+k+'_ipaddr'].substr(0, nvram['lan'+k+'_ipaddr'].lastIndexOf('.'))) == (e.ip.substr(0, e.ip.lastIndexOf('.')))) {
					e.bridge = 'br'+j;
					break;
				}
			}
		}

		/* find LANx */
		for (j = 0; j <= MAX_BRIDGE_ID; j++) {
			k = (j == 0) ? '' : j.toString();
			if (nvram['lan'+k+'_ifname'] == e.bridge && e.bridge != '') {
				e.lan = 'LAN'+j+' ';
				break;
			}
		}

		/* find WANx, proto */
		for (j = 1; j <= MAX_PORT_ID; j++) {
			k = (j == 1) ? '' : j.toString();
			if (((nvram['wan'+k+'_ifname'] == e.ifname) || (nvram['wan'+k+'_ifnameX'] == e.ifname) || (nvram['wan'+k+'_iface'] == e.ifname)) && e.ifname != '') {
				e.wan = 'WAN'+(j ? (j - 1) : '0')+' ';
				e.proto = nvram['wan'+k+'_proto'];
			}
		}
	}

/* IPV6-BEGIN */
	/* step 2: sync IPv4 Infos to IPv6 with matching mac addr (extra line/entry for IPv6 with mac, ipv6, name and lease and synced IPv4 infos) */
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		for (i2 = list.length - 1; i2 >= 0; --i2) {
			e2 = list[i2];
			if ((e.mac == e2.mac) && (e.ip != e2.ip)) { /* match mac */
				if ((e2.bridge == '') || (e2.lan == '')) { /* check infos before sync */
					e2.ifname = e.ifname;
					e2.ifstatus = e.ifstatus;
					e2.bridge = e.bridge;
					e2.freq = e.freq;
					e2.ssid = e.ssid;
					e2.mode = e.mode;
					e2.unit = e.unit;
					e2.rssi = e.rssi;
					e2.qual = e.qual;
					e2.txrx = e.txrx;
					e2.lan = e.lan;
				}
				else {
					e.ifname = e2.ifname;
					e.ifstatus = e2.ifstatus;
					e.bridge = e2.bridge;
					e.freq = e2.freq;
					e.ssid = e2.ssid;
					e.mode = e2.mode;
					e.unit = e2.unit;
					e.rssi = e2.rssi;
					e.qual = e2.qual;
					e.txrx = e2.txrx;
					e.lan = e2.lan;
				}
				/* special case: name can be empty for IPv4 OR IPv6 - sync */
				if ((e.name != '') && (e2.name == ''))
					e2.name = e.name;
				else if ((e2.name != '') && (e.name == ''))
					e.name = e2.name;
			}
		}
	}
/* IPV6-END */

	/* step 3: finish it */
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];

		if ((e.mac.match(/^(..):(..):(..)/)) && e.proto != 'pppoe' && e.proto != 'pptp' && e.proto != 'l2tp') {
			b = '<a href="javascript:searchOUI(\''+RegExp.$1+'-'+RegExp.$2+'-'+RegExp.$3+'\','+i+')" title="OUI Search">'+e.mac+'<\/a><div style="display:none" id="gW_'+i+'">&nbsp; <img src="spin.gif" alt="" style="vertical-align:middle"><\/div>'+
			    '<br><small class="pics">'+
			    '<a href="javascript:addStatic('+i+')" title="Static Lease">[SL]<\/a> '+
			    '<a href="javascript:addbwlimit('+i+')" title="BW Limiter">[BWL]<\/a> '+
			    '<a href="javascript:addRestrict('+i+')" title="Access Restriction">[AR]<\/a>';

			if (e.rssi != '')
				b += ' <a href="javascript:addWF('+i+')" title="Wireless Filter">[WLF]<\/a>';

			b += '<\/small>';
		}
		else
			b = '&nbsp;<br>&nbsp;';

		if (e.ssid != '')
			c = '<br><small>'+e.ssid+'<\/small>';
		else {
			if (e.proto == 'dhcp')
				a = 'DHCP'
			else if (e.proto == 'pppoe')
				a = 'PPPoE'
			else if (e.proto == 'static')
				a = 'Static'
			else if (e.proto == 'pptp')
				a = 'PPTP'
			else if (e.proto == 'l2tp')
				a = 'L2TP'
			else
				a = '';

			c = (a ? '<br><small>'+a+'<\/small>' : '');
		}

		a = '';
		if ((e.rssi < 0 && e.qual >= 0)) /* WL */
			a = e.ifstatus+' '+e.ifname+(e.ifname.indexOf('.') == -1 ? ' (wl'+e.unit+')' : '')+c;
		else if ((e.ifname != '' && e.name != '') || (e.ifname != ''))
			a = e.lan+e.wan+'('+e.ifname+')'+c;
		else
			e.rssi = 1; /* fake value only for checking */

		/* fix issue when disconnected WL devices are displayed (for a while) as a LAN devices */
		if (list_last.indexOf(e.mac) == -1) {
			if (e.freq != '') /* it means device is on WL, connected */
				list_last.push(e.mac);
		}
		else if (e.freq == '')
			e.rssi = 1; /* fake value only for checking */

		f = '';
		if (e.freq != '') {
			f = '<img src="wl'+(e.freq == '5 GHz' ? '50' : '24')+'.gif"'+((e.mode == 'wet' || e.mode == 'sta') ? 'style="filter:invert(1)"' : '')+' alt="" title="'+e.freq+'">';
			e.media = (e.freq == '5 GHz' ? 1 : 2);
		}
		else if (e.ifname != '' && mode != 'wet') {
			c = (e.wan != '' ? 'style="filter:invert(1)"' : '');
/* USB-BEGIN */
			if ((e.proto == 'lte') || (e.proto == 'ppp3g')) {
				f = '<img src="cell.gif"'+c+' alt="" title="LTE / 3G">';
				e.media = 3;
			}
			else
/* USB-END */
			     if (e.rssi != 1) {
				f = '<img src="eth.gif"'+c+' alt="" title="Ethernet">';
				e.media = 4;
			}
		}
		if (e.rssi == 1) {
			f = '<img src="dis.gif"'+c+' alt="" title="Disconnected">';
			e.media = 5;
		}

		this.insert(-1, e, [ a, '<div id="media_'+i+'">'+f+'<\/div>', b, (e.ip == '-' ? '' : e.ip), e.name, (e.rssi < 0 ? e.rssi+' <small>dBm<\/small>' : ''),
		                     (e.qual < 0 ? '' : '<small>'+e.qual+'<\/small> <img src="bar'+MIN(MAX(Math.floor(e.qual / 12), 1), 6)+'.gif" id="bar_'+i+'" alt="">'),
		                     e.txrx, e.lease], false);
	}
}

dg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var ra = a.getRowData();
	var rb = b.getRowData();
	var r;

	switch (col) {
	case 1:
		r = cmpInt(ra.media, rb.media);
	break;
	case 3:
		r = cmpIP(ra.ip, rb.ip);
	break;
	case 5:
		r = cmpInt(ra.rssi, rb.rssi);
	break;
	case 6:
		r = cmpInt(ra.qual, rb.qual);
	break;
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}

	if (r == 0) {
		r = cmpIP(ra.ip, rb.ip);
		if (r == 0)
			r = cmpText(ra.ifname, rb.ifname);
	}

	return this.sortAscending ? r : -r;
}

function earlyInit() {
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
					var a = wl_display_ifname(uidx);
					f.push( { title: '<b>Noise Floor<\/b> '+a.substr(0, a.indexOf('/') - 1)+'&nbsp;<b>:<\/b>', prefix: '<span id="noise'+uidx+'">', custom: wlnoise[uidx], suffix: '<\/span>&nbsp;<small>dBm<\/small>' } );
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
