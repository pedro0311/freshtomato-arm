<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Copyright (C) 2011 Deon 'PrinceAMD' Thomas 
	rate limit & connection limit from Conanxu, 
	adapted by Victek, Shibby, PrinceAMD, Phykris

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] IP/Range BW Limiter</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("bwl_enable,wan_qos_ibw,wan_qos_obw,bwl_rules,lan_ipaddr,lan_netmask,bwl_br0_enable,bwl_br0_dlr,bwl_br0_dlc,bwl_br0_ulr,bwl_br0_ulc,bwl_br0_udp,bwl_br0_tcp,bwl_br0_prio,bwl_br1_enable,bwl_br1_dlc,bwl_br1_dlr,bwl_br1_ulc,bwl_br1_ulr,bwl_br1_prio,bwl_br2_enable,bwl_br2_dlc,bwl_br2_dlr,bwl_br2_ulc,bwl_br2_ulr,bwl_br2_prio,bwl_br3_enable,bwl_br3_dlc,bwl_br3_dlr,bwl_br3_ulc,bwl_br3_ulr,bwl_br3_prio"); %>

var class_prio = [['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']];
var class_tcp = [['0','nolimit']];
var class_udp = [['0','nolimit']];
for (var i = 1; i <= 100; ++i) {
	class_tcp.push([i*10, i*10+'']);
	class_udp.push([i, i + '/s']);
}

var bwlg = new TomatoGrid();

bwlg.setup = function() {
	this.init('bwlg-grid', '', 80, [
		{ type: 'text', maxlen: 31 },
		{ type: 'text', maxlen: 8 },
		{ type: 'text', maxlen: 8 },
		{ type: 'text', maxlen: 8 },
		{ type: 'text', maxlen: 8 },
		{ type: 'select', options: class_prio },
		{ type: 'select', options: class_tcp },
		{ type: 'select', options: class_udp }]);
	this.headerSet(['IP | IP Range | MAC Address', 'DLRate', 'DLCeil', 'ULRate', 'ULCeil', 'Priority', 'TCP Limit', 'UDP Limit']);
	var bwllimitrules = nvram.bwl_rules.split('>');
	for (var i = 0; i < bwllimitrules.length; ++i) {
		var t = bwllimitrules[i].split('<');
		if (t.length == 8) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

bwlg.dataToView = function(data) {
	return [data[0],data[1]+'kbps',data[2]+'kbps',data[3]+'kbps',data[4]+'kbps',class_prio[data[5]*1][1],class_tcp[data[6]*1/10][1],class_udp[data[7]*1][1]];
}

bwlg.resetNewEditor = function() {
	var f, c, n;

	var f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	if ((c = cookie.get('addbwlimit')) != null) {
		cookie.set('addbwlimit', '', 0);
		c = c.split(',');
		if (c.length == 2) {
			f[0].value = c[0];
			f[1].value = '';
			f[2].value = '';
			f[3].value = '';
			f[4].value = '';
			f[5].selectedIndex = '2';
			f[6].selectedIndex = '0';
			f[7].selectedIndex = '0';
			return;
		}
	}

	f[0].value = '';
	f[1].value = '';
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].selectedIndex = '2';
	f[6].selectedIndex = '0';
	f[7].selectedIndex = '0';
	}

bwlg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}

	return false;
}

bwlg.existID = function(id) {
	return this.exist(0, id);
}

bwlg.existIP = function(ip) {
	if (ip == "0.0.0.0") return true;

	return this.exist(1, ip);
}

bwlg.checkRate = function(rate) {
	var s = parseInt(rate, 10);
	if (isNaN(s) || s <= 0 || s >= 100000000) return true;

	return false;
}

bwlg.checkRateCeil = function(rate, ceil) {
	var r = parseInt(rate, 10);
	var c = parseInt(ceil, 10);
	if (r > c) return true;

	return false;
}

bwlg.verifyFields = function(row, quiet) {
	var ok = 1;
	var f = fields.getAll(row);
	var s;

/*
	if (v_ip(f[0], quiet)) {
		if (this.existIP(f[0].value)) {
			ferror.set(f[0], 'duplicate IP address', quiet);
			ok = 0;
		}
	}
*/
	if (v_macip(f[0], quiet, 0, nvram.lan_ipaddr, nvram.lan_netmask)) {
		if (this.existIP(f[0].value)) {
			ferror.set(f[0], 'duplicate IP or MAC address', quiet);
			ok = 0;
		}
	}

	if (this.checkRate(f[1].value)) {
		ferror.set(f[1], 'DLRate must be between 1 and 99999999', quiet);
		ok = 0;
	}

	if (this.checkRate(f[2].value)) {
		ferror.set(f[2], 'DLCeil must be between 1 and 99999999', quiet);
		ok = 0;
	}

	if (this.checkRateCeil(f[1].value, f[2].value)) {
		ferror.set(f[2], 'DLCeil must be greater than DLRate', quiet);
		ok = 0;
	}

	if (this.checkRate(f[3].value)) {
		ferror.set(f[3], 'ULRate must be between 1 and 99999999', quiet);
		ok = 0;
	}

	if (this.checkRate(f[4].value)) {
		ferror.set(f[4], 'ULCeil must be between 1 and 99999999', quiet);
		ok = 0;
	}

	if (this.checkRateCeil(f[3].value, f[4].value)) {
		ferror.set(f[4], 'ULCeil must be greater than ULRate', quiet);
		ok = 0;
	}

	return ok;
}

function verifyFields(focused, quiet) {
	var a = !E('_f_bwl_enable').checked;
	var b = !E('_f_bwl_br0_enable').checked;
	var b1 = !E('_f_bwl_br1_enable').checked;
	var b2 = !E('_f_bwl_br2_enable').checked;
	var b3 = !E('_f_bwl_br3_enable').checked;

	E('_wan_qos_ibw').disabled = a;
	E('_wan_qos_obw').disabled = a;
	E('_f_bwl_br0_enable').disabled = a;
	E('_f_bwl_br1_enable').disabled = a;
	E('_f_bwl_br2_enable').disabled = a;
	E('_f_bwl_br3_enable').disabled = a;

	E('_bwl_br0_dlr').disabled = b || a;
	E('_bwl_br0_dlc').disabled = b || a;
	E('_bwl_br0_ulr').disabled = b || a;
	E('_bwl_br0_ulc').disabled = b || a;
	E('_bwl_br0_tcp').disabled = b || a;
	E('_bwl_br0_udp').disabled = b || a;
	E('_bwl_br0_prio').disabled = b || a;

	elem.display(PR('_wan_qos_ibw'), PR('_wan_qos_obw'), !a);
	elem.display(PR('_bwl_br0_dlr'), PR('_bwl_br0_dlc'), PR('_bwl_br0_ulr'), PR('_bwl_br0_ulc'), PR('_bwl_br0_tcp'), PR('_bwl_br0_udp'), PR('_bwl_br0_prio'), !a && !b);

	E('_bwl_br1_dlr').disabled = b1 || a;
	E('_bwl_br1_dlc').disabled = b1 || a;
	E('_bwl_br1_ulr').disabled = b1 || a;
	E('_bwl_br1_ulc').disabled = b1 || a;
	E('_bwl_br1_prio').disabled = b1 || a;
	elem.display(PR('_bwl_br1_dlr'), PR('_bwl_br1_dlc'), PR('_bwl_br1_ulr'), PR('_bwl_br1_ulc'), PR('_bwl_br1_prio'), !a && !b1);

	E('_bwl_br2_dlr').disabled = b2 || a;
	E('_bwl_br2_dlc').disabled = b2 || a;
	E('_bwl_br2_ulr').disabled = b2 || a;
	E('_bwl_br2_ulc').disabled = b2 || a;
	E('_bwl_br2_prio').disabled = b2 || a;
	elem.display(PR('_bwl_br2_dlr'), PR('_bwl_br2_dlc'), PR('_bwl_br2_ulr'), PR('_bwl_br2_ulc'), PR('_bwl_br2_prio'), !a && !b2);

	E('_bwl_br3_dlr').disabled = b3 || a;
	E('_bwl_br3_dlc').disabled = b3 || a;
	E('_bwl_br3_ulr').disabled = b3 || a;
	E('_bwl_br3_ulc').disabled = b3 || a;
	E('_bwl_br3_prio').disabled = b3 || a;
	elem.display(PR('_bwl_br3_dlr'), PR('_bwl_br3_dlc'), PR('_bwl_br3_ulr'), PR('_bwl_br3_ulc'), PR('_bwl_br3_prio'), !a && !b3);

	return 1;
}

function save() {
	if (bwlg.isEditing()) return;

	var data = bwlg.getAllData();
	var bwllimitrules = '';
	var i;

	if (data.length != 0) bwllimitrules += data[0].join('<'); 
	for (i = 1; i < data.length; ++i) {
		bwllimitrules += '>' + data[i].join('<');
	}

	var fom = E('t_fom');
	fom.bwl_enable.value = E('_f_bwl_enable').checked ? 1 : 0;
	fom.bwl_br0_enable.value = E('_f_bwl_br0_enable').checked ? 1 : 0;
	fom.bwl_br1_enable.value = E('_f_bwl_br1_enable').checked ? 1 : 0;
	fom.bwl_br2_enable.value = E('_f_bwl_br2_enable').checked ? 1 : 0;
	fom.bwl_br3_enable.value = E('_f_bwl_br3_enable').checked ? 1 : 0;
	fom.bwl_rules.value = bwllimitrules;
	form.submit(fom, 1);
}

function init() {
	bwlg.recolor();
}

function earlyInit() {
	bwlg.setup();
	verifyFields(null, true);
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="bwlimit.asp">
<input type="hidden" name="_nextwait" value="10">
<input type="hidden" name="_service" value="bwlimit-restart">
<input type="hidden" name="bwl_enable">
<input type="hidden" name="bwl_rules">
<input type="hidden" name="bwl_br0_enable">
<input type="hidden" name="bwl_br1_enable">
<input type="hidden" name="bwl_br2_enable">
<input type="hidden" name="bwl_br3_enable">

<!-- / / / -->

<div class="section-title">Bandwidth Limiter for LAN (br0)</div>
<div class="section">
	<script>
		createFieldTable('', [
		{ title: 'Enable Limiter', name: 'f_bwl_enable', type: 'checkbox', value: nvram.bwl_enable != '0' },
		{ title: 'Max Available Download <br><small>(same as used in QoS)<\/small>', indent: 2, name: 'wan_qos_ibw', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.wan_qos_ibw },
		{ title: 'Max Available Upload <br><small>(same as used in QoS)<\/small>', indent: 2, name: 'wan_qos_obw', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.wan_qos_obw }
		]);
	</script>

	<div class="tomato-grid" id="bwlg-grid"></div>
	<div>
		<ul>
			<li><b>IP Address / IP Range:</b>
			</li><li>Example: 192.168.1.5 for one IP.
			</li><li>Example: 192.168.1.4-7 for IP 192.168.1.4 to 192.168.1.7
			</li><li>Example: 4-7 for IP Range .4 to .7
			</li><li><b>The IP Range devices will share the Bandwidth</b>
			</li><li><b>MAC Address</b> Example: 00:2E:3C:6A:22:D8
		</li></ul>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Default Class for unlisted MAC / IP's in LAN (br0)</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_bwl_br0_enable', type: 'checkbox', value: nvram.bwl_br0_enable == '1'},
			{ title: 'Download rate', indent: 2, name: 'bwl_br0_dlr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br0_dlr },
			{ title: 'Download ceil', indent: 2, name: 'bwl_br0_dlc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br0_dlc },
				{ title: 'Upload rate', indent: 2, name: 'bwl_br0_ulr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br0_ulr },
			{ title: 'Upload ceil', indent: 2, name: 'bwl_br0_ulc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br0_ulc },
			{ title: 'Priority', indent: 2, name: 'bwl_br0_prio', type: 'select', options:
				[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.bwl_br0_prio },
			{ title: 'TCP Limit', indent: 2, name: 'bwl_br0_tcp', type: 'select', options:
				[['0', 'no limit'],['1', '1'],['2', '2'],['5', '5'],['10', '10'],['20', '20'],['50', '50'],['100', '100'],['200', '200'],['500', '500'],['1000', '1000']], value: nvram.bwl_br0_tcp },
			{ title: 'UDP limit', indent: 2, name: 'bwl_br0_udp', type: 'select', options:
				[['0', 'no limit'],['1', '1/s'],['2', '2/s'],['5', '5/s'],['10', '10/s'],['20', '20/s'],['50', '50/s'],['100', '100/s']], value: nvram.bwl_br0_udp }
		]);
	</script>
	<div>
		<ul>
			<li><b>Default Class</b> - IP / MAC's non included in the list will take the Default Rate/Ceiling setting
			</li><li><b>The bandwidth will be shared by all unlisted hosts in br0</b>
		</li></ul>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Default Class for LAN1 (br1)</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_bwl_br1_enable', type: 'checkbox', value: nvram.bwl_br1_enable == '1'},
			{ title: 'Download rate', indent: 2, name: 'bwl_br1_dlr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br1_dlr },
			{ title: 'Download ceil', indent: 2, name: 'bwl_br1_dlc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br1_dlc },
			{ title: 'Upload rate', indent: 2, name: 'bwl_br1_ulr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br1_ulr },
			{ title: 'Upload ceil', indent: 2, name: 'bwl_br1_ulc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br1_ulc },
			{ title: 'Priority', indent: 2, name: 'bwl_br1_prio', type: 'select', options:
				[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.bwl_br1_prio }
		]);
	</script>
	<div>
		<ul>
			<li><b>The bandwidth will be shared by all hosts in br1.</b></li>
		</ul>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Default Class for LAN2 (br2)</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_bwl_br2_enable', type: 'checkbox', value: nvram.bwl_br2_enable == '1'},
			{ title: 'Download rate', indent: 2, name: 'bwl_br2_dlr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br2_dlr },
			{ title: 'Download ceil', indent: 2, name: 'bwl_br2_dlc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br2_dlc },
			{ title: 'Upload rate', indent: 2, name: 'bwl_br2_ulr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br2_ulr },
			{ title: 'Upload ceil', indent: 2, name: 'bwl_br2_ulc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br2_ulc },
			{ title: 'Priority', indent: 2, name: 'bwl_br2_prio', type: 'select', options:
				[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.bwl_br2_prio }
		]);
	</script>
	<div>
		<ul>
			<li><b>The bandwidth will be shared by all hosts in br2.</b></li>
		</ul>
	</div>
</div>

<!-- / / / -->

<div class="section-title">Default Class for LAN3 (br3)</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable', name: 'f_bwl_br3_enable', type: 'checkbox', value: nvram.bwl_br3_enable == '1'},
			{ title: 'Download rate', indent: 2, name: 'bwl_br3_dlr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br3_dlr },
			{ title: 'Download ceil', indent: 2, name: 'bwl_br3_dlc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br3_dlc },
			{ title: 'Upload rate', indent: 2, name: 'bwl_br3_ulr', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br3_ulr },
			{ title: 'Upload ceil', indent: 2, name: 'bwl_br3_ulc', type: 'text', maxlen: 8, size: 8, suffix: ' <small>kbit/s<\/small>', value: nvram.bwl_br3_ulc },
			{ title: 'Priority', indent: 2, name: 'bwl_br3_prio', type: 'select', options:
				[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.bwl_br3_prio }
		]);
	</script>
	<div>
		<ul>
			<li><b>The bandwidth will be shared by all hosts in br3.</b></li>
		</ul>
	</div>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
