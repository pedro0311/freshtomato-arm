<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Advanced: Firewall</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript" src="tomato.js"></script>

<!-- / / / -->

<style type="text/css">
textarea {
	width: 98%;
	height: 10em;
}
</style>

<script type="text/javascript" src="debug.js"></script>

<script type="text/javascript">

//	<% nvram("block_wan,block_wan_limit,block_wan_limit_icmp,block_wan_limit_tr,nf_loopback,ne_syncookies,DSCP_fix_enable,ipv6_ipsec,multicast_pass,multicast_lan,multicast_lan1,multicast_lan2,multicast_lan3,multicast_custom,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,udpxy_enable,udpxy_stats,udpxy_clients,udpxy_port,ne_snat"); %>

function verifyFields(focused, quiet) {
/* ICMP */
	E('_f_icmp_limit').disabled = !E('_f_icmp').checked;
	E('_f_icmp_limit_icmp').disabled = (!E('_f_icmp').checked || !E('_f_icmp_limit').checked);
	E('_f_icmp_limit_traceroute').disabled = (!E('_f_icmp').checked || !E('_f_icmp_limit').checked);

	var enable_mcast = E('_f_multicast').checked;
	E('_f_multicast_lan').disabled = ((!enable_mcast) || (nvram.lan_ifname.length < 1));
	E('_f_multicast_lan1').disabled = ((!enable_mcast) || (nvram.lan1_ifname.length < 1));
	E('_f_multicast_lan2').disabled = ((!enable_mcast) || (nvram.lan2_ifname.length < 1));
	E('_f_multicast_lan3').disabled = ((!enable_mcast) || (nvram.lan3_ifname.length < 1));
	if(nvram.lan_ifname.length < 1)
		E('_f_multicast_lan').checked = false;
	if(nvram.lan1_ifname.length < 1)
		E('_f_multicast_lan1').checked = false;
	if(nvram.lan2_ifname.length < 1)
		E('_f_multicast_lan2').checked = false;
	if(nvram.lan3_ifname.length < 1)
		E('_f_multicast_lan3').checked = false;

	var mcast_lan = E('_f_multicast_lan').checked;
	var mcast_lan1 = E('_f_multicast_lan1').checked;
	var mcast_lan2 = E('_f_multicast_lan2').checked;
	var mcast_lan3 = E('_f_multicast_lan3').checked;
	var mcast_custom_enable = 0;
/* disable multicast_custom textarea if lanX is checked / selected */
	E('_multicast_custom').disabled = ((!enable_mcast) ||  (mcast_lan) || (mcast_lan1) || (mcast_lan2) || (mcast_lan3));
/* check if more than 50 charactars are in the textarea (no plausibility test) */
	if (!E('_multicast_custom').disabled && v_length('_multicast_custom', 1, 50, 2048)) {
		mcast_custom_enable = 1;
	} else {
		mcast_custom_enable = 0;
	}
/* IGMP proxy enable checked but no lanX checked and no custom config */
	if ((enable_mcast) && (!mcast_lan) && (!mcast_lan1) && (!mcast_lan2) && (!mcast_lan3) && (!mcast_custom_enable)) {
		ferror.set('_f_multicast', 'IGMP proxy must be enabled in least one LAN bridge OR you have to use custom configuration', quiet);
		return 0;
/* IGMP proxy enable checked but custom config / textarea length not OK */
	} else if ((enable_mcast) && (mcast_custom_enable) && !v_length('_multicast_custom', quiet, 0, 2048)) {
		return 0;
/* clear */
	} else {
		ferror.clear('_f_multicast');
	}

	E('_f_udpxy_stats').disabled = !E('_f_udpxy_enable').checked;
	E('_f_udpxy_clients').disabled = !E('_f_udpxy_enable').checked;
	E('_f_udpxy_port').disabled = !E('_f_udpxy_enable').checked;

	return 1;
}

function save() {
	var fom;

	if (!verifyFields(null, 0)) return;

	fom = E('t_fom');
	fom.block_wan.value = E('_f_icmp').checked ? 0 : 1;
	fom.block_wan_limit.value = E('_f_icmp_limit').checked? 1 : 0;
	fom.block_wan_limit_icmp.value = E('_f_icmp_limit_icmp').value;
	fom.block_wan_limit_tr.value = E('_f_icmp_limit_traceroute').value;

	fom.ne_syncookies.value = E('_f_syncookies').checked ? 1 : 0;
	fom.DSCP_fix_enable.value = E('_f_DSCP_fix_enable').checked ? 1 : 0;
	fom.ipv6_ipsec.value = E('_f_ipv6_ipsec').checked ? 1 : 0;
	fom.multicast_pass.value = E('_f_multicast').checked ? 1 : 0;
	fom.multicast_lan.value = E('_f_multicast_lan').checked ? 1 : 0;
	fom.multicast_lan1.value = E('_f_multicast_lan1').checked ? 1 : 0;
	fom.multicast_lan2.value = E('_f_multicast_lan2').checked ? 1 : 0;
	fom.multicast_lan3.value = E('_f_multicast_lan3').checked ? 1 : 0;
	fom.udpxy_enable.value = E('_f_udpxy_enable').checked ? 1 : 0;
	fom.udpxy_stats.value = E('_f_udpxy_stats').checked ? 1 : 0;
	fom.udpxy_clients.value = E('_f_udpxy_clients').value;
	fom.udpxy_port.value = E('_f_udpxy_port').value;
	form.submit(fom, 1);
}

function init() {
	var elements = document.getElementsByClassName("new_window");
	for (var i = 0; i < elements.length; i++) if (elements[i].nodeName.toLowerCase()==="a")
		addEvent(elements[i], "click", function(e) { cancelDefaultAction(e); window.open(this,"_blank"); } );
}
</script>

</head>
<body onload="init()">
<form id="t_fom" method="post" action="tomato.cgi">
<table id="container" cellspacing="0">
<tr><td colspan="2" id="header">
	<div class="title">Tomato</div>
	<div class="version">Version <% version(); %></div>
</td></tr>
<tr id="body"><td id="navi"><script type="text/javascript">navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="advanced-firewall.asp">
<input type="hidden" name="_service" value="firewall-restart">

<input type="hidden" name="block_wan">
<input type="hidden" name="block_wan_limit">
<input type="hidden" name="block_wan_limit_icmp">
<input type="hidden" name="block_wan_limit_tr">
<input type="hidden" name="ne_syncookies">
<input type="hidden" name="DSCP_fix_enable">
<input type="hidden" name="ipv6_ipsec">
<input type="hidden" name="multicast_pass">
<input type="hidden" name="multicast_lan">
<input type="hidden" name="multicast_lan1">
<input type="hidden" name="multicast_lan2">
<input type="hidden" name="multicast_lan3">
<input type="hidden" name="udpxy_enable">
<input type="hidden" name="udpxy_stats">
<input type="hidden" name="udpxy_clients">
<input type="hidden" name="udpxy_port">

<div class="section-title">Firewall</div>
<div class="section">
<script type="text/javascript">
createFieldTable('', [
	{ title: 'Respond to ICMP ping', name: 'f_icmp', type: 'checkbox', value: nvram.block_wan == '0' },
	{ title: 'Limits per second', name: 'f_icmp_limit', type: 'checkbox', value: nvram.block_wan_limit != '0' },
	{ title: 'ICMP', indent: 2, name: 'f_icmp_limit_icmp', type: 'text', maxlen: 3, size: 3, suffix: ' <small> request per second<\/small>', value: fixInt(nvram.block_wan_limit_icmp || 1, 1, 300, 5) },
	{ title: 'Traceroute', indent: 2, name: 'f_icmp_limit_traceroute', type: 'text', maxlen: 3, size: 3, suffix: ' <small> request per second<\/small>', value: fixInt(nvram.block_wan_limit_tr || 5, 1, 300, 5) },
	null,
	{ title: 'Enable SYN cookies', name: 'f_syncookies', type: 'checkbox', value: nvram.ne_syncookies != '0' },
	{ title: 'Enable DSCP Fix', name: 'f_DSCP_fix_enable', type: 'checkbox', value: nvram.DSCP_fix_enable != '0', suffix: ' <small>Fixes Comcast incorrect DSCP<\/small>' },
	{ title: 'IPv6 IPSec Passthrough', name: 'f_ipv6_ipsec', type: 'checkbox', value: nvram.ipv6_ipsec != '0' }
]);
</script>
</div>

<div class="section-title">NAT</div>
<div class="section">
<script type="text/javascript">
createFieldTable('', [
	{ title: 'NAT loopback', name: 'nf_loopback', type: 'select', options: [[0,'All'],[1,'Forwarded Only'],[2,'Disabled']], value: fixInt(nvram.nf_loopback, 0, 2, 1) },
	{ title: 'NAT target', name: 'ne_snat', type: 'select', options: [[0,'MASQUERADE'],[1,'SNAT']], value: nvram.ne_snat }
]);
</script>
</div>

<div class="section-title">Multicast</div>
<div class="section">
<script type="text/javascript">
createFieldTable('', [
	{ title: 'Enable IGMP proxy', name: 'f_multicast', type: 'checkbox', value: nvram.multicast_pass == '1' },
	{ title: 'LAN', indent: 2, name: 'f_multicast_lan', type: 'checkbox', value: (nvram.multicast_lan == '1') },
	{ title: 'LAN1', indent: 2, name: 'f_multicast_lan1', type: 'checkbox', value: (nvram.multicast_lan1 == '1') },
	{ title: 'LAN2', indent: 2, name: 'f_multicast_lan2', type: 'checkbox', value: (nvram.multicast_lan2 == '1') },
	{ title: 'LAN3', indent: 2, name: 'f_multicast_lan3', type: 'checkbox', value: (nvram.multicast_lan3 == '1') },
	{ title: '<a href="https://github.com/pali/igmpproxy" class="new_window">IGMP proxy<\/a><br />Custom configuration', name: 'multicast_custom', type: 'textarea', value: nvram.multicast_custom },
	null,
	{ title: 'Enable Udpxy', name: 'f_udpxy_enable', type: 'checkbox', value: (nvram.udpxy_enable == '1') },
	{ title: 'Enable client statistics', indent: 2, name: 'f_udpxy_stats', type: 'checkbox', value: (nvram.udpxy_stats == '1') },
	{ title: 'Max clients', indent: 2, name: 'f_udpxy_clients', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.udpxy_clients || 3, 1, 5000, 3) },
	{ title: 'Udpxy port', indent: 2, name: 'f_udpxy_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.udpxy_port, 4022) }

]);
</script>
</div>

<!-- / / / -->

<div class="section-title">IGMP proxy notes</div>
<div class="section">
<ul>
	<li><b>LAN / LAN1 / LAN2 / LAN3</b> - Add interface br0 / br1 / br2 / br3 to igmp.conf (Ex.: phyint br0 downstream ratelimit 0 threshold 1).</li>
	<li><b>Custom configuration</b> - Use custom config for IGMP proxy instead of tomato default config. You must define one (or more) upstream interface(s) and one or more downstream interfaces. Refer to the <a href="https://github.com/pali/igmpproxy/blob/master/igmpproxy.conf" class="new_window">IGMP proxy example configuration</a> and <a href="https://github.com/pali/igmpproxy/commit/b55e0125c79fc9dbc95c6d6ab1121570f0c6f80f" class="new_window">IGMP proxy commit b55e0125c79fc9d</a> for details.</li>
	<li><b>Other hints</b> - For error messages please check the <a href="status-log.asp">log file</a>.</li>
</ul>
</div>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save()">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</td></tr>
</table>
</form>
<script type="text/javascript">verifyFields(null, 1);</script>
</body>
</html>