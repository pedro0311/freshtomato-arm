<!DOCTYPE html>
<!--
	Tomato MySQL GUI
	Copyright (C) 2014 Hyzoom, bwq518@gmail.com
	http://openlinksys.info
	For use with Tomato Shibby Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] MySQL Database Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("mysql_enable,mysql_sleep,mysql_check,mysql_check_time,mysql_binary,mysql_binary_custom,mysql_usb_enable,mysql_dlroot,mysql_datadir,mysql_tmpdir,mysql_server_custom,mysql_port,mysql_allow_anyhost,mysql_init_rootpass,mysql_username,mysql_passwd,mysql_key_buffer,mysql_max_allowed_packet,mysql_thread_stack,mysql_thread_cache_size,mysql_init_priv,mysql_table_open_cache,mysql_sort_buffer_size,mysql_read_buffer_size,mysql_query_cache_size,mysql_read_rnd_buffer_size,mysql_max_connections,nginx_port"); %>

//	<% usbdevices(); %>

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
	E('_mysql_notice').innerHTML = 'MySQL is currently '+(!isup.mysqld ? 'stopped' : 'running')+' ';
	E('_mysql_button').value = (isup.mysqld ? 'Stop' : 'Start')+' Now';
	E('_mysql_button').setAttribute('onclick', 'javascript:toggle(\'mysql\', '+isup.mysqld+');');
	E('_mysql_button').disabled = 0;
	E('_mysql_interface').disabled = isup.mysqld ? 0 : 1;
}

function toggle(service, isup) {
	if (changed && !confirm("There are unsaved changes. Continue anyway?"))
		return;

	E('_mysql_button').disabled = 1;
	E('_mysql_interface').disabled = 1;

	var fom = E('t_fom');
	fom._service.value = service+(isup ? '-stop' : '-start');
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'service.cgi');
}

var usb_disk_list = new Array();

function refresh_usb_disk() {
	var i, j, k, a, b, c, e, s, desc, d, parts, p;
	var partcount;
	var list = [];
	for (i = 0; i < list.length; ++i) {
		list[i].type = '';
		list[i].host = '';
		list[i].vendor = '';
		list[i].product = '';
		list[i].serial = '';
		list[i].discs = [];
		list[i].is_mounted = 0;
	}
	for (i = usbdev.length - 1; i >= 0; --i) {
		a = usbdev[i];
		e = {
			type: a[0],
			host: a[1],
			vendor: a[2],
			product: a[3],
			serial: a[4],
			discs: a[5],
			is_mounted: a[6]
		};
		list.push(e);
	}
	partcount = 0;
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (e.discs) {
			for (j = 0; j <= e.discs.length - 1; ++j) {
				d = e.discs[j];
				parts = d[1];
				for (k = 0; k <= parts.length - 1; ++k) {
					p = parts[k];
					if ((p) && (p[1] >= 1) && (p[3] != 'swap')) {
						usb_disk_list[partcount] = new Array();
						usb_disk_list[partcount][0] = p[2];
						usb_disk_list[partcount][1] = 'Partition '+p[0]+' mounted on '+p[2]+' ('+p[3]+ ' - '+doScaleSize(p[6])+ ' available, total '+doScaleSize(p[5])+')';
						partcount++;
					}
				}
			}
		}
	}
	list = [];
}

function verifyFields(focused, quiet) {
	if (focused)
		changed = 1;

	var ok = 1;

	var a = E('_f_mysql_enable').checked;
	var o = E('_f_mysql_check').checked;
	var u = E('_f_mysql_usb_enable').checked;
	var r = E('_f_mysql_init_rootpass').checked;

	E('_f_mysql_check').disabled = !a;
	E('_mysql_check_time').disabled = !a || !o;
	E('_mysql_sleep').disabled = !a;
	E('_mysql_binary').disabled = !a;
	E('_f_mysql_init_priv').disabled = !a;
	E('_f_mysql_init_rootpass').disabled = !a;
	E('_mysql_username').disabled = true;
	E('_mysql_passwd').disabled = !a || !r;
	E('_mysql_server_custom').disabled = !a;
	E('_f_mysql_usb_enable').disabled = !a;
	E('_mysql_dlroot').disabled = !a || !u;
	E('_mysql_datadir').disabled = !a;
	E('_mysql_tmpdir').disabled = !a;
	E('_mysql_port').disabled = !a;
	E('_f_mysql_allow_anyhost').disabled = !a;
	E('_mysql_key_buffer').disabled = !a;
	E('_mysql_max_allowed_packet').disabled = !a;
	E('_mysql_thread_stack').disabled = !a;
	E('_mysql_thread_cache_size').disabled = !a;
	E('_mysql_table_open_cache').disabled = !a;
	E('_mysql_sort_buffer_size').disabled = !a;
	E('_mysql_read_buffer_size').disabled = !a;
	E('_mysql_query_cache_size').disabled = !a;
	E('_mysql_read_rnd_buffer_size').disabled = !a;
	E('_mysql_max_connections').disabled = !a;
	
	var p = (E('_mysql_binary').value == 'custom');
	elem.display('_mysql_binary_custom', p && a);

	elem.display('_mysql_dlroot', u);

	var x;
	if (r && a)
		x = '';
	else
		x = 'none';

	PR(E('_mysql_username')).style.display = x;
	PR(E('_mysql_passwd')).style.display = x;

	var e = E('_mysql_passwd');
	if (e.value.trim() == '') {
		ferror.set(e, 'Password can not be NULL value', quiet);
		ok = 0;
	}

	return ok;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');
	fom.mysql_enable.value = fom._f_mysql_enable.checked ? 1 : 0;
	fom.mysql_check.value = fom._f_mysql_check.checked ? 1 : 0;
	fom.mysql_usb_enable.value = fom._f_mysql_usb_enable.checked ? 1 : 0;
	fom.mysql_init_priv.value = fom._f_mysql_init_priv.checked ? 1 : 0;
	fom.mysql_init_rootpass.value = fom._f_mysql_init_rootpass.checked ? 1 : 0;
	fom.mysql_allow_anyhost.value = fom._f_mysql_allow_anyhost.checked ? 1 : 0;

	if (fom.mysql_enable.value)
		fom._service.value = 'mysqlgui-restart';
	else
		fom._service.value = 'mysql-stop';

	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
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
<div id="ident"><% ident(); %> | <script>wikiLink();</script></div>

<!-- / / / -->

<input type="hidden" name="_nextpage" value="web-mysql.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="mysql_enable">
<input type="hidden" name="mysql_check">
<input type="hidden" name="mysql_usb_enable">
<input type="hidden" name="mysql_init_priv">
<input type="hidden" name="mysql_init_rootpass">
<input type="hidden" name="mysql_allow_anyhost">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_mysql_notice"></span>
		<input type="button" id="_mysql_button">
		<input type="button" id="_mysql_interface" value="Open admin interface in new tab" class="new_window" onclick="window.open('http://'+location.hostname+':'+nvram.nginx_port+'/adminer.php')">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Basic Settings</div>
<div class="section" id="config-section1">
	<script>
		refresh_usb_disk();

		createFieldTable('', [
			{ title: 'Enable Server on Start', name: 'f_mysql_enable', type: 'checkbox', value: nvram.mysql_enable == 1 },
			{ title: 'MySQL binary path', multi: [
				{ name: 'mysql_binary', type: 'select', options: [
					['internal','Internal (/usr/bin)'],
					['optware','Optware (/opt/bin)'],
					['custom','Custom'] ], value: nvram.mysql_binary },
				{ name: 'mysql_binary_custom', type: 'text', maxlen: 40, size: 40, value: nvram.mysql_binary_custom }
			] },
			{ title: 'Keep alive', name: 'f_mysql_check', type: 'checkbox', value: nvram.mysql_check == 1 },
			{ title: 'Check alive every', indent: 2, name: 'mysql_check_time', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_check_time, suffix: '&nbsp; <small>minutes (range: 1 - 55; default: 1)<\/small>' },
			{ title: 'Delay at startup', name: 'mysql_sleep', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_sleep, suffix: '&nbsp; <small>seconds (range: 1 - 60; default: 2)<\/small>' },
			{ title: 'MySQL listen port', name: 'mysql_port', type: 'text', maxlen: 5, size: 7, value: nvram.mysql_port, suffix: '&nbsp; <small> default: 3306<\/small>' },
			{ title: 'Allow Anyhost to access', name: 'f_mysql_allow_anyhost', type: 'checkbox', value: nvram.mysql_allow_anyhost == 1, suffix: '&nbsp; <small>Allowed any hosts to access database server.<\/small>' },
			{ title: 'Re-init priv. table', name: 'f_mysql_init_priv', type: 'checkbox', value: nvram.mysql_init_priv== 1, suffix: '&nbsp; <small>If checked, privileges table will be forced to re-initialize by mysql_install_db.<\/small>' },
			{ title: 'Re-init root password', name: 'f_mysql_init_rootpass', type: 'checkbox', value: nvram.mysql_init_rootpass == 1, suffix: '&nbsp; <small>If checked, root password will be forced to re-initialize.<\/small>' },
			{ title: 'root user name', name: 'mysql_username', type: 'text', maxlen: 32, size: 16, value: nvram.mysql_username, suffix: '&nbsp; <small>user name connected to server.(default: root)<\/small>' },
			{ title: 'root password', name: 'mysql_passwd', type: 'password', maxlen: 32, size: 16, peekaboo: 1, value: nvram.mysql_passwd, suffix: '&nbsp; <small>not allowed NULL.(default: admin)<\/small>' },
			{ title: 'Enable USB Partition', multi: [
				{ name: 'f_mysql_usb_enable', type: 'checkbox', value: nvram.mysql_usb_enable == 1, suffix: '&nbsp; ' },
				{ name: 'mysql_dlroot', type: 'select', options: usb_disk_list, value: nvram.mysql_dlroot} ] },
			{ title: 'Data dir.', indent: 2, name: 'mysql_datadir', type: 'text', maxlen: 50, size: 40, value: nvram.mysql_datadir, suffix: '&nbsp; <small>Directory name under mounted partition.<\/small>' },
			{ title: 'Tmp dir.', indent: 2, name: 'mysql_tmpdir', type: 'text', maxlen: 50, size: 40, value: nvram.mysql_tmpdir, suffix: '&nbsp; <small>Directory name under mounted partition.<\/small>' }
		]);
	</script>
	<ul>
		<li><b>Enable MySQL server</b> - Caution! - If your router only has 32MB of RAM, you'll have to use swap</li>
		<li><b>MySQL binary path</b> - Path to the directory containing mysqld etc. Do not include program name (/mysqld)</li>
		<li><b>Keep alive</b> - If enabled, mysqld will be checked at the specified interval and will re-launch after a crash</li>
		<li><b>Data and tmp dir</b> - Attention! Must not use NAND for datadir and tmpdir</li>
	</ul>
</div>

<!-- / / / -->

<div class="section-title">Advanced Settings</div>
<div class="section" id="config-section2">
	<script>
		createFieldTable('', [
			{ title: 'Key buffer', name: 'mysql_key_buffer', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_key_buffer, suffix: '&nbsp; <small>MB (range: 1 - 1024; default: 8)<\/small>' },
			{ title: 'Max allowed packet', name: 'mysql_max_allowed_packet', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_max_allowed_packet, suffix: '&nbsp; <small>MB (range: 1 - 1024; default: 4)<\/small>' },
			{ title: 'Thread stack', name: 'mysql_thread_stack', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_thread_stack, suffix: '&nbsp; <small>KB (range: 1 - 1024000; default: 192)<\/small>' },
			{ title: 'Thread cache size', name: 'mysql_thread_cache_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_thread_cache_size, suffix: '&nbsp; <small>(range: 1 - 999999; default: 8)<\/small>' },
			{ title: 'Table open cache', name: 'mysql_table_open_cache', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_table_open_cache, suffix: '&nbsp; <small>(range: 1 - 999999; default: 4)<\/small>' },
			{ title: 'Query cache size', name: 'mysql_query_cache_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_query_cache_size, suffix: '&nbsp; <small>MB (range: 0 - 1024; default: 16)<\/small>' },
			{ title: 'Sort buffer size', name: 'mysql_sort_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_sort_buffer_size, suffix: '&nbsp; <small>KB (range: 0 - 1024000; default: 128)<\/small>' },
			{ title: 'Read buffer size', name: 'mysql_read_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_read_buffer_size, suffix: '&nbsp; <small>KB (range: 0 - 1024000; default: 128)<\/small>' },
			{ title: 'Read rand buffer size', name: 'mysql_read_rnd_buffer_size', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_read_rnd_buffer_size, suffix: '&nbsp; <small>KB (range: 1 - 1024000; default: 256)<\/small>' },
			{ title: 'Max connections', name: 'mysql_max_connections', type: 'text', maxlen: 10, size: 10, value: nvram.mysql_max_connections, suffix: '&nbsp; <small>(range: 0 - 999999; default: 100)<\/small>' },
			{ title: 'MySQL server custom config', name: 'mysql_server_custom', type: 'textarea', value: nvram.mysql_server_custom }
		]);
	</script>
	<ul>
		<li><b>MySQL Server custom config</b> - input like: param=value, e.g. connect_timeout=10</li>
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
