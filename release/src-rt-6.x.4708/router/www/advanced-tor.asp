<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: TOR Project</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("tor_enable,tor_solve_only,tor_socksport,tor_transport,tor_dnsport,tor_datadir,tor_users,tor_ports,tor_ports_custom,tor_custom,tor_iface,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

function verifyFields(focused, quiet) {
	var ok = 1;

	var a = E('_f_tor_enable').checked;
	var b = E('_f_tor_solve_only').checked;
	var o = (E('_tor_iface').value == 'custom');
	var p = (E('_tor_ports').value == 'custom');

	E('_tor_socksport').disabled = !a;
	E('_tor_transport').disabled = !a;
	E('_tor_dnsport').disabled = !a;
	E('_tor_datadir').disabled = !a;
	E('_tor_iface').disabled = !a || b;
	E('_tor_ports').disabled = !a || b;
	E('_tor_custom').disabled = !a;

	elem.display('_tor_users', o && a && !b);
	elem.display('_tor_ports_custom', p && a && !b);

	var bridge = E('_tor_iface');
	if(nvram.lan_ifname.length < 1)
		bridge.options[0].disabled = true;
	if(nvram.lan1_ifname.length < 1)
		bridge.options[1].disabled = true;
	if(nvram.lan2_ifname.length < 1)
		bridge.options[2].disabled = true;
	if(nvram.lan3_ifname.length < 1)
		bridge.options[3].disabled = true;

	var s = E('_tor_custom');

	if (s.value.indexOf('SocksBindAddress') != -1) {
		ferror.set(s, 'Cannot set "SocksBindAddress" option here.', quiet);
		ok = 0;
	}

	if (s.value.indexOf('AllowUnverifiedNodes') != -1) {
		ferror.set(s, 'Cannot set "AllowUnverifiedNodes" option here.', quiet);
		ok = 0;
	}

	if (s.value.indexOf('Log') != -1) {
		ferror.set(s, 'Cannot set "Log" option here.', quiet);
		ok = 0;
	}

	if (s.value.indexOf('DataDirectory') != -1) {
		ferror.set(s, 'Cannot set "DataDirectory" option here. You can set it in Tomato GUI', quiet);
		ok = 0;
	}

	if (s.value.indexOf('TransPort') != -1) {
		ferror.set(s, 'Cannot set "TransPort" option here. You can set it in Tomato GUI', quiet);
		ok = 0;
	}

	if (s.value.indexOf('TransListenAddress') != -1) {
		ferror.set(s, 'Cannot set "TransListenAddress" option here.', quiet);
		ok = 0;
	}

	if (s.value.indexOf('DNSPort') != -1) {
		ferror.set(s, 'Cannot set "DNSPort" option here. You can set it in Tomato GUI', quiet);
		ok = 0;
	}

	if (s.value.indexOf('DNSListenAddress') != -1) {
		ferror.set(s, 'Cannot set "DNSListenAddress" option here.', quiet);
		ok = 0;
	}

	if (s.value.indexOf('User') != -1) {
		ferror.set(s, 'Cannot set "User" option here.', quiet);
		ok = 0;
	}

	return ok;
}

function save() {
	if (verifyFields(null, 0) == 0) return;

	var fom = E('t_fom');
	fom.tor_enable.value = E('_f_tor_enable').checked ? 1 : 0;
	fom.tor_solve_only.value = E('_f_tor_solve_only').checked ? 1 : 0;

	if (fom.tor_enable.value == 0) {
		fom._service.value = 'tor-stop';
	}
	else {
		fom._service.value = 'tor-restart,firewall-restart'; 
	}
	form.submit('t_fom', 1);
}
</script>
</head>

<body>
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-tor.asp">
<input type="hidden" name="_service" value="tor-restart">
<input type="hidden" name="tor_enable">
<input type="hidden" name="tor_solve_only">

<!-- / / / -->

<div class="section-title">TOR Settings</div>
<div class="section" id="config-section">
	<script>
		createFieldTable('', [
			{ title: 'Enable Tor', name: 'f_tor_enable', type: 'checkbox', value: nvram.tor_enable == '1' },
			null,
			{ title: 'Socks Port', name: 'tor_socksport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_socksport, 9050) },
			{ title: 'Trans Port', name: 'tor_transport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_transport, 9040) },
			{ title: 'DNS Port', name: 'tor_dnsport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_dnsport, 9053) },
			{ title: 'Data Directory', name: 'tor_datadir', type: 'text', maxlen: 24, size: 28, value: nvram.tor_datadir },
			null,
			{ title: 'Only solve .onion/.exit domains', name: 'f_tor_solve_only', type: 'checkbox', value: nvram.tor_solve_only == '1' },
			{ title: 'Redirect all users from', multi: [
				{ name: 'tor_iface', type: 'select', options: [['br0','LAN0 (br0)'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)'],['custom','Selected IP`s']], value: nvram.tor_iface },
				{ name: 'tor_users', type: 'text', maxlen: 512, size: 64, value: nvram.tor_users } ] },
			{ title: 'Redirect TCP Ports', multi: [
				{ name: 'tor_ports', type: 'select', options: [['80','HTTP only (TCP 80)'],['80,443','HTTP/HTTPS (TCP 80,443)'],['custom','Selected Ports']], value: nvram.tor_ports },
				{ name: 'tor_ports_custom', type: 'text', maxlen: 512, size: 64, value: nvram.tor_ports_custom } ] },
			null,
			{ title: 'Custom Configuration', name: 'tor_custom', type: 'textarea', value: nvram.tor_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Enable Tor</b> - Be patient. Starting the Tor client can take from several seconds to several minutes.</li>
		<li><b>Selected IP`s</b> - ex: 1.2.3.4,1.1.0/24,1.2.3.1-1.2.3.4</li>
		<li><b>Selected Ports</b> - ex: one port (80), few ports (80,443,8888), range of ports (80:88), mix (80,8000:9000,9999)</li>
		<li><b style="text-decoration:underline">Caution!</b> - If your router has only 32MB of RAM, you'll have to use swap.</li>
	</ul>
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
<script>verifyFields(null, true);</script>
</body>
</html>
