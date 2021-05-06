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
<title>[<% ident(); %>] Advanced: DHCP / DNS</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("dnsmasq_q,ipv6_service,ipv6_radvd,ipv6_dhcpd,ipv6_lease_time,ipv6_fast_ra,dhcpd_dmdns,dns_addget,dhcpd_gwmode,dns_intcpt,dhcpd_slt,dhcpc_minpkt,dnsmasq_custom,dnsmasq_onion_support,dhcpd_lmax,dhcpc_custom,dns_norebind,dns_fwd_local,dns_priv_override,dhcpd_static_only,dnsmasq_debug,dnssec_enable,dnscrypt_proxy,dnscrypt_priority,dnscrypt_port,dnscrypt_resolver,dnscrypt_log,dnscrypt_manual,dnscrypt_provider_name,dnscrypt_provider_key,dnscrypt_resolver_address,dnscrypt_ephemeral_keys,stubby_proxy,stubby_priority,stubby_log,stubby_dnssec,stubby_force_tls13,stubby_port,wan_wins"); %>

</script>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
var cprefix = 'advanced_dhcpdns';

var up = new TomatoRefresh('isup.jsx', '', 5);

up.refresh = function(text) {
	isup = {};
	try {
		eval(text);
	}
	catch (ex) {
		isup = {};
	}
}

/* STUBBY-BEGIN */
var up_servers_arr = [<% stubby_presets("dot"); %>];
var st_resolvers_arr = '<% nv("stubby_resolvers"); %>';
var MAX_UPSTREAM_SERVERS = 8;

function active_resolvers(ip, port, domain, pinset) {
	if (st_resolvers_arr.length) {
		var s = st_resolvers_arr.split('<');
		for (var j = 0; j < s.length; ++j) {
			if (!s[j].length)
				continue;
			var row = s[j].split('>');
			if (row.length == 4 && ip == row[0] && port == row[1] && domain == row[2] && pinset == row[3])
				return 1;
		}
	}
	return 0;
}
/* STUBBY-END */

if ((isNaN(nvram.dhcpd_lmax)) || ((nvram.dhcpd_lmax *= 1) < 1))
	nvram.dhcpd_lmax = 255;

function verifyFields(focused, quiet) {
	var vis = { }, v;

/* STUBBY-BEGIN */
/* DNSCRYPT-BEGIN */
	E('_f_stubby_proxy').disabled = (E('_f_dnscrypt_proxy').checked);
	E('_f_dnscrypt_proxy').disabled = (E('_f_stubby_proxy').checked);
/* DNSCRYPT-END */
/* STUBBY-END */
/* DNSCRYPT-BEGIN */
	v = E('_f_dnscrypt_proxy').checked;
	vis._dnscrypt_priority = v;
	vis._dnscrypt_port = v;
	vis._dnscrypt_log = v;
	vis._f_dnscrypt_manual = v;
	vis._f_dnscrypt_ephemeral_keys = v;
	v = E('_f_dnscrypt_proxy').checked && E('_f_dnscrypt_manual').checked;
	vis._dnscrypt_provider_name = v;
	vis._dnscrypt_provider_key = v;
	vis._dnscrypt_resolver_address = v;
	vis._dnscrypt_resolver = (E('_f_dnscrypt_proxy').checked && !E('_f_dnscrypt_manual').checked);
/* DNSCRYPT-END */
/* STUBBY-BEGIN */
	v = E('_f_stubby_proxy').checked;
	vis._f_dns_priv_override = v;
	vis._stubby_priority = v;
	vis._stubby_log = v;
	vis._stubby_port = v;
	vis._stubby_servers = v;
	vis._f_stubby_force_tls13 = v;
	vis._stubby_dnssec_1 = v;
/* STUBBY-END */

	for (var a in vis) {
		var b = E(a);
		var c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = (c ? 'table-row' : 'none');
	}

	v = (E('_f_dhcpd_sltsel').value == 1);
	elem.display('_dhcpd_sltman', v);

	if ((v) && (!v_range('_f_dhcpd_slt', quiet, 1, 43200)))
		return 0;
	if (!v_length('_dnsmasq_custom', quiet, 0, 4096))
		return 0;
	if (!v_range('_dhcpd_lmax', quiet, 1, 0xFFFF))
		return 0;
/* IPV6-BEGIN */
	if (!v_range('_f_ipv6_lease_time', quiet, 1, 720))
		return 0;
/* IPV6-END */
	if (!v_length('_dhcpc_custom', quiet, 0, 256))
		return 0;
/* STUBBY-BEGIN */
	if (!v_port('_stubby_port', quiet))
		return 0;

	var count = 0, e, s;
	var reg = /^_upstream_active_/;
	var els = document.getElementsByTagName('input');
	var suff = ' in imported upstream servers file';
	for (var i = els.length; i--;) {
		if (reg.test(els[i].id) && !els[i].disabled) {
			var id = els[i].id.replace(reg, '');

			if (els[i].checked) /* count checked */
				count++;

			s = E('_upstream_server_'+id).value;
			e = '_upstream_ip_'+id;
			if (!v_ip(e, 1)
/* IPV6-BEGIN */
			                && !v_ipv6_addr(e, 1)
/* IPV6-END */
			) {
				if (!quiet)
					alert('Invalid IP ('+E(e).value+') for '+s+suff);
				return 0;
			}

			e = '_upstream_port_'+id;
			if (E(e).value && !v_port(e, 1)) {
				if (!quiet)
					alert('Invalid port ('+E(e).value+') for '+s+suff);
				return 0;
			}

			e = '_upstream_domain_'+id;
			if (!v_domain(e, 1)) {
				if (!quiet)
					alert('Invalid domain ('+E(e).value+') for '+s+suff);
				return 0;
			}
		}

	}
	if (count > MAX_UPSTREAM_SERVERS) {
		if (focused) {
			if (!quiet)
				alert('Maximum of '+MAX_UPSTREAM_SERVERS+' upstream servers can be used at the same time');

			focused.checked = false;
		}

		return 0;
	}
/* STUBBY-END */

	/* IP address, blank -> 0.0.0.0 */
	if (!v_dns('_wan_wins', quiet))
		return 0;

	return 1;
}

function save() {
	if (!verifyFields(null, false))
		return;

	var fom = E('t_fom');
	var a = fom._f_dhcpd_sltsel.value;

	fom.dhcpd_dmdns.value = fom._f_dhcpd_dmdns.checked ? 1 : 0;
	fom.dhcpd_slt.value = (a != 1) ? a : fom._f_dhcpd_slt.value;
	fom.dhcpd_gwmode.value = fom._f_dhcpd_gwmode.checked ? 1 : 0;
	fom.dhcpc_minpkt.value = fom._f_dhcpc_minpkt.checked ? 1 : 0;
	fom.dhcpd_static_only.value = fom._f_dhcpd_static_only.checked ? 1 : 0;
	fom.dns_addget.value = fom._f_dns_addget.checked ? 1 : 0;
	fom.dns_norebind.value = fom._f_dns_norebind.checked ? 1 : 0;
	fom.dns_fwd_local.value = fom._f_dns_fwd_local.checked ? 1 : 0;
	fom.dns_intcpt.value = fom._f_dns_intcpt.checked ? 1 : 0;
	fom.dnsmasq_debug.value = fom._f_dnsmasq_debug.checked ? 1 : 0;
/* TOR-BEGIN */
	fom.dnsmasq_onion_support.value = fom._f_dnsmasq_onion_support.checked ? 1 : 0;
/* TOR-END */
/* IPV6-BEGIN */
	fom.ipv6_radvd.value = fom._f_ipv6_radvd.checked ? 1 : 0;
	fom.ipv6_dhcpd.value = fom._f_ipv6_dhcpd.checked ? 1 : 0;
	fom.ipv6_fast_ra.value = fom._f_ipv6_fast_ra.checked ? 1 : 0;
	fom.ipv6_lease_time.value = fom._f_ipv6_lease_time.value;
/* IPV6-END */
	fom.dnsmasq_q.value = 0;
	if (fom.f_dnsmasq_q4.checked)
		fom.dnsmasq_q.value |= 1;
/* IPV6-BEGIN */
	if (fom.f_dnsmasq_q6.checked)
		fom.dnsmasq_q.value |= 2;
	if (fom.f_dnsmasq_qr.checked)
		fom.dnsmasq_q.value |= 4;
/* IPV6-END */
/* DNSSEC-BEGIN */
	fom.dnssec_enable.value = fom.f_dnssec_enable.checked ? 1 : 0;
/* DNSSEC-END */
/* DNSCRYPT-BEGIN */
	fom.dnscrypt_proxy.value = fom.f_dnscrypt_proxy.checked ? 1 : 0;
	fom.dnscrypt_manual.value = fom.f_dnscrypt_manual.checked ? 1 : 0;
	fom.dnscrypt_ephemeral_keys.value = fom.f_dnscrypt_ephemeral_keys.checked ? 1 : 0;
/* DNSCRYPT-END */
/* STUBBY-BEGIN */
	fom.stubby_proxy.value = fom.f_stubby_proxy.checked ? 1 : 0;
	fom.dns_priv_override.value = fom._f_dns_priv_override.checked ? 1 : 0;
	fom.stubby_force_tls13.value = fom._f_stubby_force_tls13.checked ? 1 : 0;
	fom.stubby_dnssec.value = (fom._stubby_dnssec_1.checked ? 1 : (fom._stubby_dnssec_0.checked ? 0 : 2));

	var count = 0, stubby_list = '', e;
	var reg = /^_upstream_active_/;
	var els = document.getElementsByTagName('input');

	for (var i = els.length; i--;) {
		if (reg.test(els[i].id) && !els[i].disabled) {
			var id = els[i].id.replace(reg, '');

			if (els[i].checked) {
				if (count > 8) /* max 8 allowed */
					break;

				stubby_list +='<'+E('_upstream_ip_'+id).value+'>';
				e = E('_upstream_port_'+id).value;
				if (e != '853')
					stubby_list += e;

				stubby_list +='>'+E('_upstream_domain_'+id).value+'>'+E('_upstream_pinset_'+id).value;

				count++;
			}
		}
	}
	if (stubby_list.length)
		fom.stubby_resolvers.value = stubby_list;
/* STUBBY-END */

	if ((fom.dhcpc_minpkt.value != nvram.dhcpc_minpkt) || (fom.dhcpc_custom.value != nvram.dhcpc_custom)) {
		nvram.dhcpc_minpkt = fom.dhcpc_minpkt.value;
		nvram.dhcpc_custom = fom.dhcpc_custom.value;
		fom._service.value = '*'; /* special case: restart all */
	}
	else
		fom._service.value = 'dnsmasq-restart'; /* always restart dnsmasq */

	if ((fom.dns_intcpt.value != nvram.dns_intcpt)) {
		nvram.dns_intcpt = fom.dns_intcpt.value;
		if (fom._service.value != '*')
			fom._service.value += ',firewall-restart'; /* special case: restart FW */
	}

	if ((fom.wan_wins.value != nvram.wan_wins)) { /* special case: restart vpnservers/pptpd if up */
		nvram.wan_wins = fom.wan_wins.value;

		if (fom._service.value != '*') {
/* OPENVPN-BEGIN */
			if (isup.vpnserver1)
				fom._service.value += ',vpnserver1-restart';
			if (isup.vpnserver2)
				fom._service.value += ',vpnserver2-restart';
/* OPENVPN-END */
/* PPTPD-BEGIN */
			if (isup.pptpd)
				fom._service.value += ',pptpd-restart';
/* PPTPD-END */
		}
	}

	form.submit(fom, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix + '_notes_vis')) != null) && (c == '1'))
		toggleVisibility(cprefix, "notes");

	up.initPage(250, 5);
	eventHandler();
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

<input type="hidden" name="_nextpage" value="advanced-dhcpdns.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="dhcpd_dmdns">
<input type="hidden" name="dhcpd_slt">
<input type="hidden" name="dhcpc_minpkt">
<input type="hidden" name="dhcpd_static_only">
<input type="hidden" name="dhcpd_gwmode">
<input type="hidden" name="dns_addget">
<input type="hidden" name="dns_norebind">
<input type="hidden" name="dns_fwd_local">
<input type="hidden" name="dns_intcpt">
<!-- IPV6-BEGIN -->
<input type="hidden" name="ipv6_radvd">
<input type="hidden" name="ipv6_dhcpd">
<input type="hidden" name="ipv6_fast_ra">
<input type="hidden" name="ipv6_lease_time">
<!-- IPV6-END -->
<input type="hidden" name="dnsmasq_q">
<input type="hidden" name="dnsmasq_debug">
<!-- TOR-BEGIN -->
<input type="hidden" name="dnsmasq_onion_support">
<!-- TOR-END -->
<!-- DNSSEC-BEGIN -->
<input type="hidden" name="dnssec_enable">
<!-- DNSSEC-END -->
<!-- DNSCRYPT-BEGIN -->
<input type="hidden" name="dnscrypt_proxy">
<input type="hidden" name="dnscrypt_manual">
<input type="hidden" name="dnscrypt_ephemeral_keys">
<!-- DNSCRYPT-END -->
<!-- STUBBY-BEGIN -->
<input type="hidden" name="stubby_proxy">
<input type="hidden" name="stubby_resolvers">
<input type="hidden" name="stubby_dnssec">
<input type="hidden" name="stubby_force_tls13">
<input type="hidden" name="dns_priv_override">
<!-- STUBBY-END -->

<!-- / / / -->

<div class="section-title">DHCP / DNS Client (WAN)</div>
<div class="section">
	<script>
		createFieldTable('noclose', [
/* DNSSEC-BEGIN */
			{ title: 'Enable DNSSEC support', name: 'f_dnssec_enable', type: 'checkbox', suffix: '&nbsp; <small>DNS servers must support DNSSEC<\/small>', value: (nvram.dnssec_enable == 1) }
/* DNSSEC-END */
/* STUBBY-BEGIN */
			, { title: 'DNSSEC validation method', multi: [
				{ suffix: '&nbsp; Stubby&nbsp;&nbsp;&nbsp;', name: 'f_stubby_dnssec', id: '_stubby_dnssec_1', type: 'radio', value: nvram.stubby_dnssec == 1 },
				{ suffix: '&nbsp; Dnsmasq&nbsp;&nbsp;&nbsp;', name: 'f_stubby_dnssec', id: '_stubby_dnssec_0', type: 'radio', value: nvram.stubby_dnssec == 0 },
				{ suffix: '&nbsp; Server Only&nbsp;', name: 'f_stubby_dnssec', id: '_stubby_dnssec_2', type: 'radio', value: nvram.stubby_dnssec == 2 } ]}
/* STUBBY-END */
/* DNSCRYPT-BEGIN */
			, { title: 'Use dnscrypt-proxy', name: 'f_dnscrypt_proxy', type: 'checkbox', value: nvram.dnscrypt_proxy == 1 },
				{ title: 'Ephemeral Keys', indent: 2, name: 'f_dnscrypt_ephemeral_keys', type: 'checkbox', suffix: '&nbsp; <small>warning: this option requires extra CPU cycles!<\/small>', value: nvram.dnscrypt_ephemeral_keys == 1 },
				{ title: 'Manual Entry', indent: 2, name: 'f_dnscrypt_manual', type: 'checkbox', value: nvram.dnscrypt_manual == 1 },
				{ title: '<a href="https://dnscrypt.info/public-servers" title="Resolver details" class="new_window">Resolver<\/a>', indent: 2, name: 'dnscrypt_resolver', type: 'select', options: _dnscrypt_resolvers_, value: nvram.dnscrypt_resolver },
				{ title: 'Resolver Address', indent: 2, name: 'dnscrypt_resolver_address', type: 'text', maxlen: 50, size: 25, value: nvram.dnscrypt_resolver_address },
				{ title: 'Provider Name', indent: 2, name: 'dnscrypt_provider_name', type: 'text', maxlen: 60, size: 25, value: nvram.dnscrypt_provider_name },
				{ title: 'Provider Public Key', indent: 2, name: 'dnscrypt_provider_key', type: 'text', maxlen: 80, size: 25, value: nvram.dnscrypt_provider_key },
				{ title: 'Priority', indent: 2, name: 'dnscrypt_priority', type: 'select', options: [['1','Strict-Order'],['2','No-Resolv'],['0','None']], suffix: '&nbsp; <small style="color:red">warning: set to No-Resolv to only use dnscrypt-proxy resolvers!<\/small>', value: nvram.dnscrypt_priority },
				{ title: 'Local Port', indent: 2, name: 'dnscrypt_port', type: 'text', maxlen: 5, size: 7, value: nvram.dnscrypt_port },
				{ title: 'Log Level', indent: 2, name: 'dnscrypt_log', type: 'text', maxlen: 2, size: 5, value: nvram.dnscrypt_log }
/* DNSCRYPT-END */
		]);
/* STUBBY-BEGIN */
		createFieldTable('noopen,noclose', [
			{ title: 'Use Stubby', name: 'f_stubby_proxy', type: 'checkbox', value: nvram.stubby_proxy == 1 }
		]);

		W('<tr><td class="title indent2">Upstream resolvers<br> (max. 8)<\/td><td class="content" id="_stubby_servers"><table class="tomato-grid" id="stubby-grid">');
		W('<tr class="header"><td class="co1">On<\/td><td class="co2">Server<\/td><\/tr><\/tr>');

		var ip, port, server, domain, pinset, active, type, t;
		for (var i = 0; i < up_servers_arr.length; ++i) {
			type = up_servers_arr[i][5];

			if ((type.indexOf('IPv6') != -1) && (nvram.ipv6_service == ''))
				continue;

			server = '['+type+'] '+up_servers_arr[i][0];
			ip = up_servers_arr[i][1];
			port = up_servers_arr[i][2];
			domain = up_servers_arr[i][3];
			pinset = up_servers_arr[i][4];
			active = active_resolvers(ip, port, domain, pinset);
			tclass = (i & 1) ? 'even' : 'odd';
			t = server+'\n'+(ip ? 'Auth Domain: '+domain+'\nIP Addr: '+ip+'\n' : '')+'Port: '+(port ? port : '853');

			W('<tr class="'+tclass+'">\n'+
			   '<td class="co1">'+
			    '<input type="checkbox" name="upstream_active_'+i+'" id="_upstream_active_'+i+'" '+(active ? ' checked="checked"' : '')+' onclick="verifyFields(this, 0)">'+
			    '<input type="hidden" name="upstream_server_'+i+'" id="_upstream_server_'+i+'" value="'+server+'">'+
			    '<input type="hidden" name="upstream_ip_'+i+'" id="_upstream_ip_'+i+'" value="'+ip+'">'+
			    '<input type="hidden" name="upstream_port_'+i+'" id="_upstream_port_'+i+'" value="'+port+'">'+
			    '<input type="hidden" name="upstream_domain_'+i+'" id="_upstream_domain_'+i+'" value="'+domain+'">'+
			    '<input type="hidden" name="upstream_pinsetv_'+i+'" id="_upstream_pinset_'+i+'" value="'+pinset+'">'+
			   '<\/td>\n'+
			   '<td class="co2"'+(ip ? ' title="'+t+'" style="cursor:help"' : '')+'>'+server+'<\/td>\n'+
			  '<\/tr>\n');
		}

		W('<\/table><\/td><\/tr>');

		createFieldTable('noopen,noclose', [
				{ title: 'Prevent client auto DoH', indent: 2, name: 'f_dns_priv_override', type: 'checkbox', value: nvram.dns_priv_override == 1 },
				{ title: 'Priority', indent: 2, name: 'stubby_priority', type: 'select', options: [['1','Strict-Order'],['2','No-Resolv'],['0','None']], suffix: '&nbsp; <small style="color:red">warning: set to No-Resolv to only use Stubby resolvers!<\/small>', value: nvram.stubby_priority },
				{ title: 'Local Port', indent: 2, name: 'stubby_port', type: 'text', maxlen: 5, size: 7, value: nvram.stubby_port },
				{ title: 'Log Level', indent: 2, name: 'stubby_log', type: 'select',  options: [['0','Emergency'],['1','Alert'],['2','Critical'],['3','Error'],['4','Warning*'],['5','Notice'],['6','Info'],['7','Debug']],
					value: nvram.stubby_log, suffix: '&nbsp; <small>*default<\/small>' },
				{ title: 'Force TLS1.3', indent: 2, name: 'f_stubby_force_tls13', type: 'checkbox', suffix: '&nbsp; <small>note: works with OpenSSL >= 1.1.1 only<\/small>', value: nvram.stubby_force_tls13 == 1 },
			null
		]);
/* STUBBY-END */
		createFieldTable('noopen', [
			{ title: 'WINS <small>(for DHCP)<\/small>', name: 'wan_wins', type: 'text', maxlen: 15, size: 17, value: nvram.wan_wins },
			{ title: 'Enable DNS Rebind protection', name: 'f_dns_norebind', type: 'checkbox', value: nvram.dns_norebind == 1 },
			{ title: 'Forward local domain queries to upstream DNS', name: 'f_dns_fwd_local', type: 'checkbox', value: nvram.dns_fwd_local == 1 },
			null,
			{ title: 'DHCPC Options', name: 'dhcpc_custom', type: 'textarea', value: nvram.dhcpc_custom },
			{ title: 'Reduce packet size', name: 'f_dhcpc_minpkt', type: 'checkbox', value: nvram.dhcpc_minpkt == 1 }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">DHCP / DNS Server (LAN)</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Use internal DNS', name: 'f_dhcpd_dmdns', type: 'checkbox', value: nvram.dhcpd_dmdns == 1 },
				{ title: 'Debug Mode', indent: 2, name: 'f_dnsmasq_debug', type: 'checkbox', value: nvram.dnsmasq_debug == 1 },
			{ title: 'Use received DNS with user-entered DNS', name: 'f_dns_addget', type: 'checkbox', value: nvram.dns_addget == 1 },
			{ title: 'Intercept DNS port', name: 'f_dns_intcpt', type: 'checkbox', value: nvram.dns_intcpt == 1 },
			{ title: 'Use user-entered gateway if WAN is disabled', name: 'f_dhcpd_gwmode', type: 'checkbox', value: nvram.dhcpd_gwmode == 1 },
			{ title: 'Ignore DHCP requests from unknown devices', name: 'f_dhcpd_static_only', type: 'checkbox', value: nvram.dhcpd_static_only == 1 },
/* TOR-BEGIN */
			{ title: 'Solve .onion using Tor<br>(<a href="advanced-tor.asp" class="new_window">enable Tor first<\/a>)', name: 'f_dnsmasq_onion_support', type: 'checkbox', value: nvram.dnsmasq_onion_support == 1 },
/* TOR-END */
			{ title: 'Maximum active DHCP leases', name: 'dhcpd_lmax', type: 'text', maxlen: 5, size: 8, value: nvram.dhcpd_lmax },
			{ title: 'Static lease time', multi: [
				{ name: 'f_dhcpd_sltsel', type: 'select', options: [[0,'Same as normal lease time'],[-1,'"Infinite"'],[1,'Custom']], value: (nvram.dhcpd_slt < 1) ? nvram.dhcpd_slt : 1 },
				{ name: 'f_dhcpd_slt', type: 'text', maxlen: 5, size: 8, prefix: '<span id="_dhcpd_sltman"> ', suffix: ' <i>(minutes)<\/i><\/span>', value: (nvram.dhcpd_slt >= 1) ? nvram.dhcpd_slt : 3600 } ] },
/* IPV6-BEGIN */
			{ title: 'Announce IPv6 on LAN (SLAAC)', name: 'f_ipv6_radvd', type: 'checkbox', value: nvram.ipv6_radvd == 1 },
			{ title: 'Announce IPv6 on LAN (DHCP)', name: 'f_ipv6_dhcpd', type: 'checkbox', value: nvram.ipv6_dhcpd == 1 },
			{ title: 'Fast RA mode', name: 'f_ipv6_fast_ra', type: 'checkbox', value: nvram.ipv6_fast_ra == 1 },
			{ title: 'DHCP IPv6 lease time', name: 'f_ipv6_lease_time', type: 'text', maxlen: 3, size: 8, suffix: ' <small> (in hours)<\/small>', value: nvram.ipv6_lease_time || 12, hidden: nvram.ipv6_service == 'native-pd' },
/* IPV6-END */
			{ title: 'Mute dhcpv4 logging', name: 'f_dnsmasq_q4', type: 'checkbox', value: (nvram.dnsmasq_q & 1) },
/* IPV6-BEGIN */
			{ title: 'Mute dhcpv6 logging', name: 'f_dnsmasq_q6', type: 'checkbox', value: (nvram.dnsmasq_q & 2) },
			{ title: 'Mute RA logging', name: 'f_dnsmasq_qr', type: 'checkbox', value: (nvram.dnsmasq_q & 4) },
/* IPV6-END */
			{ title: '<a href="http://www.thekelleys.org.uk/" class="new_window">Dnsmasq<\/a><br>Custom configuration', name: 'dnsmasq_custom', type: 'textarea', value: nvram.dnsmasq_custom }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href='javascript:toggleVisibility(cprefix,"notes");'><span id="sesdiv_notes_showhide">(Click here to show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<i>DHCP / DNS Client (WAN):</i><br>
	<ul>
<!-- DNSSEC-BEGIN -->
		<li><b>Enable DNSSEC support</b> - Ensures that DNS lookups haven't been hijacked by a malicious third party when querying a DNSSEC-enabled domain. Make sure your WAN/ISP/Stubby DNS are DNSSEC-compatible, otherwise DNS lookups will always fail.</li>
<!-- DNSSEC-END -->
<!-- STUBBY-BEGIN -->
		<li><b>DNSSEC validation method</b> - choose between dnsmasq/Stubby/server only DNSSEC validation.</li>
<!-- STUBBY-END -->
<!-- DNSCRYPT-BEGIN -->
		<li><b>Use dnscrypt-proxy</b> - Wraps unmodified DNS traffic between a client and a DNS resolver in a cryptographic construction in order to detect forgery. Uses the DNSCrypt (v1) protocol.</li>
<!-- DNSCRYPT-END -->
<!-- STUBBY-BEGIN -->
		<li><b>Use Stubby</b> - Acts as a local DNS Privacy stub resolver (using DNS-over-TLS). Stubby encrypts DNS queries sent to a DNS Privacy resolver increasing end user privacy. You can use your own custom config file (/etc/stubby/stubby_alt.yml)</li>
		<li><b>Prevent client auto DoH</b> - Some clients like Firefox will automatically switch to DNS over HTTPS, bypassing your preferred DNS servers. This option may prevent that.</li>
<!-- STUBBY-END -->
		<li><b>WINS <small>(for DHCP)</small></b> - The Windows Internet Naming Service manages interaction of each PC with the Internet. If you use a WINS server, enter IP Address of server here.</li>
		<li><b>Enable DNS Rebind protection</b> - Enabling this will protect your LAN against DNS rebind attacks, however it will prevent upstream DNS servers from resolving queries to any non-routable IP (for example, 192.168.1.1).</li>
		<li><b>DHCPC Options</b> - Extra options for the DHCP client.</li>
		<li><b>Reduce packet size</b> - Self-explanatory.</li>
	</ul>
	<br>
	<i>DHCP / DNS Server (LAN):</i><br>
	<ul>
		<li><b>Use internal DNS</b> - Allow dnsmasq to be your DNS server on LAN.</li>
		<li><b>Use received DNS with user-entered DNS</b> - Add DNS servers received from your WAN connection to the static DNS server list (see <a href="basic-network.asp">Network</a> configuration).</li>
		<li><b>Prevent DNS-rebind attacks</b> - Enable DNS rebinding protection on Dnsmasq.</li>
		<li><b>Intercept DNS port</b> - Any DNS requests/packets sent out to UDP/TCP port 53 are redirected to the internal DNS server. Currently only IPv4 DNS is intercepted.</li>
		<li><b>Use user-entered gateway if WAN is disabled</b> - DHCP will use the IP address of the router as the default gateway on each LAN.</li>
		<li><b>Ignore DHCP requests (...)</b> - Dnsmasq will ignore DHCP requests to only MAC addresses listed on the <a href="basic-static.asp">Static DHCP/ARP</a> page won't be able to obtain an IP address through DHCP.</li>
		<li><b>Static lease time</b> - Absolute maximum amount of time allowed for any DHCP lease to be valid.</li>
<!-- IPV6-BEGIN -->
		<li><b>Fast RA mode</b> - Forces dnsmasq to be always in frequent RA mode. (Recommendation: enable also "Mute RA logging" option)</li>
<!-- IPV6-END -->
		<li><b>Custom configuration</b> - Extra options to be added to the Dnsmasq configuration file.</li>
	</ul>
	<br>
	<i>Other relevant notes/hints:</i><br>
	<ul>
		<li>The contents of file /etc/dnsmasq.custom are also added to the end of Dnsmasq's configuration file (if it exists).</li>
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
<script>verifyFields(null, 1);</script>
</body>
</html>
