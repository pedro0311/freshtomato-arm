<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2009 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] PPTP: Client</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("pptp_client_eas,pptp_client_usewan,pptp_client_peerdns,pptp_client_mtuenable,pptp_client_mtu,pptp_client_mruenable,pptp_client_mru,pptp_client_nat,pptp_client_srvip,pptp_client_srvsub,pptp_client_srvsubmsk,pptp_client_username,pptp_client_passwd,pptp_client_mppeopt,pptp_client_crypt,pptp_client_custom,pptp_client_dfltroute,pptp_client_stateless"); %>

</script>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
var up = new TomatoRefresh('isup.jsx?_http_id=<% nv(http_id); %>', '', 5);

up.refresh = function(text) {
	isup = {};
	try {
		eval(text);
	}
	catch (ex) {
		isup = {};
	}
	show();
}

var changed = 0;

function show() {
	var d = isup.pptpclient;
	var e = E('_pptpclient_button');

	e.value = (d ? 'Stop' : 'Start')+' Now';
	e.setAttribute('onclick', 'javascript:toggle(\'pptpclient\', '+d+');');
	e.disabled = 0;
}

function toggle(service, isup) {
	if (changed && !confirm('There are unsaved changes. Continue anyway?'))
		return;

	E('_'+service+'_button').disabled = 1;

	var fom = E('t_fom');
	fom._service.value = service+(isup ? '-stop' : '-start');
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'service.cgi');
}

function verifyFields(focused, quiet) {
	var ok = 1;

	elem.display(PR('_pptp_client_srvsub'), PR('_pptp_client_srvsubmsk'), !E('_f_pptp_client_dfltroute').checked);

	var f = E('_pptp_client_mtuenable').value == '0';
	if (f)
		E('_pptp_client_mtu').value = '1400';

	E('_pptp_client_mtu').disabled = f;

	f = E('_pptp_client_mruenable').value == '0';
	if (f)
		E('_pptp_client_mru').value = '1400';

	E('_pptp_client_mru').disabled = f;

	if (!v_range('_pptp_client_mtu', quiet, 576, 1500))
		ok = 0;
	if (!v_range('_pptp_client_mru', quiet, 576, 1500))
		ok = 0;
	if (!v_ip('_pptp_client_srvip', true) && !v_domain('_pptp_client_srvip', true)) {
		ferror.set(E('_pptp_client_srvip'), "Invalid server address.", quiet);
		ok = 0;
	}
	if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsub', true)) {
		ferror.set(E('_pptp_client_srvsub'), "Invalid subnet address.", quiet);
		ok = 0;
	}
	if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsubmsk', true)) {
		ferror.set(E('_pptp_client_srvsubmsk'), "Invalid netmask address.", quiet);
		ok = 0;
	}

	changed |= ok;

	return ok;
}

function save() {
	if (!verifyFields(null, false))
		return;

	var fom = E('t_fom');

	fom.pptp_client_eas.value = fom._f_pptp_client_eas.checked ? 1 : 0;
	fom.pptp_client_nat.value = fom._f_pptp_client_nat.checked ? 1 : 0;
	fom.pptp_client_dfltroute.value = fom._f_pptp_client_dfltroute.checked ? 1 : 0;
	fom.pptp_client_stateless.value = fom._f_pptp_client_stateless.checked ? 1 : 0;
	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
	changed = 0;
	up.initPage(250, 5);
}
</script>
</head>

<body onload="init()">
<form id="t_fom" method="post" action="/tomato.cgi">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="vpn-pptp.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="pptp_client_eas">
<input type="hidden" name="pptp_client_nat">
<input type="hidden" name="pptp_client_dfltroute">
<input type="hidden" name="pptp_client_stateless">

<!-- / / / -->

<div class="section-title">PPTP Client Configuration</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Start with WAN', name: 'f_pptp_client_eas', type: 'checkbox', value: nvram.pptp_client_eas != 0 },
			{ title: 'Bind to', name: 'pptp_client_usewan', type: 'select',
				options: [['wan','WAN0'],['wan2','WAN1'],
/* MULTIWAN-BEGIN */
					['wan3','WAN2'],['wan4','WAN3'],
/* MULTIWAN-END */
					['none','none']], value: nvram.pptp_client_usewan },
			{ title: 'Server Address', name: 'pptp_client_srvip', type: 'text', maxlen: 50, size: 27, value: nvram.pptp_client_srvip },
			{ title: 'Username: ', name: 'pptp_client_username', type: 'text', maxlen: 50, size: 54, value: nvram.pptp_client_username },
			{ title: 'Password: ', name: 'pptp_client_passwd', type: 'password', maxlen: 50, size: 54, value: nvram.pptp_client_passwd },
			{ title: 'Encryption', name: 'pptp_client_crypt', type: 'select', options: [['0','Auto'],['1','None'],['2','Maximum (128 bit only)'],['3','Required (128, 56 or 40 bit)']], value: nvram.pptp_client_crypt },
			{ title: 'Stateless MPPE connection', name: 'f_pptp_client_stateless', type: 'checkbox', value: nvram.pptp_client_stateless != 0 },
			{ title: 'Accept DNS configuration', name: 'pptp_client_peerdns', type: 'select', options: [[0,'Disabled'],[1,'Yes'],[2,'Exclusive']], value: nvram.pptp_client_peerdns },
			{ title: 'Redirect Internet traffic', name: 'f_pptp_client_dfltroute', type: 'checkbox', value: nvram.pptp_client_dfltroute != 0 },
			{ title: 'Remote subnet / netmask', multi: [
				{ name: 'pptp_client_srvsub', type: 'text', maxlen: 15, size: 17, value: nvram.pptp_client_srvsub },
				{ name: 'pptp_client_srvsubmsk', type: 'text', maxlen: 15, size: 17, prefix: ' /&nbsp', value: nvram.pptp_client_srvsubmsk } ] },
			{ title: 'Create NAT on tunnel', name: 'f_pptp_client_nat', type: 'checkbox', value: nvram.pptp_client_nat != 0 },
			{ title: 'MTU', multi: [
				{ name: 'pptp_client_mtuenable', type: 'select', options: [['0','Default'],['1','Manual']], value: nvram.pptp_client_mtuenable },
				{ name: 'pptp_client_mtu', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mtu } ] },
			{ title: 'MRU', multi: [
				{ name: 'pptp_client_mruenable', type: 'select', options: [['0','Default'],['1','Manual']], value: nvram.pptp_client_mruenable },
				{ name: 'pptp_client_mru', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mru } ] },
			{ title: 'Custom Configuration', name: 'pptp_client_custom', type: 'textarea', value: nvram.pptp_client_custom }
		]);
	</script>
	<div class="vpn-start-stop"><input type="button" value="" onclick="" id="_pptpclient_button"></div>
</div>

<!-- / / / -->

<div class="section-title">Notes</div>
<div class="section">
	<ul>
		<li><b>Do not change (and save)</b> the settings when client <b>is running</b> - you may end up with a downed firewall or broken routing table!</li>
		<li>In case of connection problems, reduce the MTU and/or MRU values.</li>
		<li>To boost connection performance, you can try to increase MTU/MRU values.</li>
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
<script>earlyInit();</script>
</body>
</html>
