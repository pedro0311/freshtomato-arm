<!DOCTYPE html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	NGINX Web Server Management Control
	Ofer Chen (roadkill AT tomatoraf dot com)
	Vicente Soriano (victek AT tomatoraf dot com)
	Copyright (C) 2013 http://www.tomatoraf.com
	
	For use with Tomato Firmware only.
	No part of this file can be used or modified without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Web Server Menu</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("nginx_enable,nginx_php,nginx_keepconf,nginx_port,nginx_upload,nginx_remote,nginx_fqdn,nginx_docroot,nginx_priority,nginx_custom,nginx_httpcustom,nginx_servercustom,nginx_user,nginx_phpconf,nginx_override,nginx_overridefile"); %>

</script>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
var cprefix = 'web_nginx';

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
	E('_nginxfp_notice').innerHTML = 'NGINX is currently '+(!isup.nginx ? 'stopped' : 'running')+' ';
	E('_nginxfp_button').value = (isup.nginx ? 'Stop' : 'Start')+' Now';
	E('_nginxfp_button').setAttribute('onclick', 'javascript:toggle(\'nginxfp\', '+isup.nginx+');');
	E('_nginxfp_button').disabled = 0;
}

function toggle(service, isup) {
	if (changed && !confirm("There are unsaved changes. Continue anyway?"))
		return;

	E('_'+service+'_button').disabled = 1;

	var fom = E('t_fom');
	fom._service.value = service+(isup ? '-stop' : '-start');
	fom._nofootermsg.value = 1;

	form.submit(fom, 1, 'service.cgi');
}

function verifyFields(focused, quiet) {
	if (focused)
		changed = 1;

	var ok = 1;

	var a = E('_f_nginx_enable').checked;
	var b = E('_f_nginx_override').checked;

	E('_f_nginx_php').disabled = !a ;
	E('_f_nginx_keepconf').disabled = !a || b;
	E('_nginx_port').disabled = !a || b;
	E('_nginx_upload').disabled = !a || b;
	E('_f_nginx_remote').disabled = !a;
	E('_nginx_fqdn').disabled = !a || b;
	E('_nginx_docroot').disabled = !a || b;
	E('_nginx_priority').disabled = !a || b;
	E('_nginx_custom').disabled = !a || b;
	E('_nginx_httpcustom').disabled = !a || b;
	E('_nginx_servercustom').disabled = !a || b;
	E('_nginx_user').disabled = !a;
	E('_nginx_phpconf').disabled = !a || b;
	E('_f_nginx_override').disabled = !a;
	E('_nginx_overridefile').disabled = !a || !b;

	return ok;
}

function save() {
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');
	fom.nginx_enable.value = fom._f_nginx_enable.checked ? 1 : 0;

	if (fom.nginx_enable.value) {
		fom.nginx_php.value = fom.f_nginx_php.checked ? 1 : 0;
		fom.nginx_keepconf.value = fom.f_nginx_keepconf.checked ? 1 : 0;
		fom.nginx_remote.value = fom.f_nginx_remote.checked ? 1 : 0;
		fom.nginx_override.value = fom.f_nginx_override.checked ? 1 : 0;
		fom._service.value = 'nginxfp-restart';
	}
	else
		fom._service.value = 'nginxfp-stop';

	fom._nofootermsg.value = 0;

	form.submit(fom, 1);

	changed = 0;
}

function earlyInit() {
	show();
	verifyFields(null, 1);
}

function init() {
	var c;
	if (((c = cookie.get(cprefix+'_notes_vis')) != null) && (c == '1'))
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

<input type="hidden" name="_nextpage" value="web-nginx.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="nginx_enable">
<input type="hidden" name="nginx_php">
<input type="hidden" name="nginx_keepconf">
<input type="hidden" name="nginx_remote">
<input type="hidden" name="nginx_override">

<!-- / / / -->

<div class="section-title">Status</div>
<div class="section">
	<div class="fields">
		<span id="_nginxfp_notice"></span>
		<input type="button" id="_nginxfp_button">
	</div>
</div>

<!-- / / / -->

<div class="section-title">Basic Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: 'Enable Server on Start', name: 'f_nginx_enable', type: 'checkbox', value: nvram.nginx_enable == '1'},
			{ title: 'Enable PHP support', name: 'f_nginx_php', type: 'checkbox', value: nvram.nginx_php == '1' },
			{ title: 'Run As', name: 'nginx_user', type: 'select', options: [['root','Root'],['nobody','Nobody']], value: nvram.nginx_user },
			{ title: 'Keep Config Files', name: 'f_nginx_keepconf', type: 'checkbox', value: nvram.nginx_keepconf == '1' },
			{ title: 'Web Server Port', name: 'nginx_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.nginx_port, 85), suffix: '<small> default: 85<\/small>' },
			{ title: 'Upload file size limit', name: 'nginx_upload', type: 'text', maxlen: 5, size: 7, value: nvram.nginx_upload, suffix: '<small> MB<\/small>'},
			{ title: 'Allow Remote Access', name: 'f_nginx_remote', type: 'checkbox', value: nvram.nginx_remote == '1' },
			{ title: 'Web Server Name', name: 'nginx_fqdn', type: 'text', maxlen: 255, size: 20, value: nvram.nginx_fqdn },
			{ title: 'Document Root Path', name: 'nginx_docroot', type: 'text', maxlen: 255, size: 40, value: nvram.nginx_docroot, suffix: '<small>&nbsp;/index.html / index.htm / index.php / /_h5ai/public/index.php<\/small>' },
			{ title: 'Server Priority', name: 'nginx_priority', type: 'text', maxlen: 8, size:3, value: nvram.nginx_priority, suffix:'<small> Max. Perfor: -20, Min.Perfor: 19, default: 10<\/small>' }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Advanced Settings</div>
<div class="section">
	<script>
		createFieldTable('', [
			{ title: '<a href="http://wiki.nginx.org/Configuration" class="new_window">NGINX<\/a><br>HTTP Section<br>Custom configuration', name: 'nginx_httpcustom', type: 'textarea', value: nvram.nginx_httpcustom },
			{ title: '<a href="http://wiki.nginx.org/Configuration" class="new_window">NGINX<\/a><br>SERVER Section<br>Custom configuration', name: 'nginx_servercustom', type: 'textarea', value: nvram.nginx_servercustom },
			{ title: '<a href="http://wiki.nginx.org/Configuration" class="new_window">NGINX<\/a><br>Custom configuration', name: 'nginx_custom', type: 'textarea', value: nvram.nginx_custom },
			{ title: '<a href="http://php.net/manual/en/ini.php" class="new_window">PHP<\/a><br>Custom configuration', name: 'nginx_phpconf', type: 'textarea', value: nvram.nginx_phpconf },
			null,
			{ title: 'Use user config file', name: 'f_nginx_override', type: 'checkbox', value: nvram.nginx_override == '1', suffix: '<small> User config file will be used, some of GUI settings will be ignored<\/small>' },
			{ title: 'User config file path', name: 'nginx_overridefile', type: 'text', maxlen: 255, size: 40, value: nvram.nginx_overridefile }
		]);
	</script>
</div>

<!-- / / / -->

<div class="section-title">Notes <small><i><a href='javascript:toggleVisibility(cprefix,"notes");'><span id="sesdiv_notes_showhide">(Show)</span></a></i></small></div>
<div class="section" id="sesdiv_notes" style="display:none">
	<ul>
		<li><b> Status Button:</b> Quick Start-Stop Service. Enable Web Server must be checked to modify settings.</li>
		<li><b> Enable Server on Start:</b> Check to activate the Web Server.</li>
		<li><b> Enable PHP support:</b> Check to enable PHP support (php-cgi).</li>
		<li><b> Run As:</b> Select user used to start nginx and php-cgi daemon.</li>
		<li><b> Keep Config Files:</b> Have you modified the configuration file manually? Tick this box and changes will be maintained.</li> 
		<li><b> Web Server Port:</b> The Port used by the Web Server to be accessed. Check conflict when the port is used by other services.</li>
		<li><b> Allow remote access:</b> This option will open the Web Server GUI port from the WAN side. Service will be accessed from the internet.</li>
		<li><b> Web Server Name:</b> Name that will appear on top of your Internet Browser.</li>
		<li><b> Document Root Path:</b> The path in your router where documents are stored (ex: /tmp/mnt/HDD/www).</li>
		<li><b> NGINX Custom Configuration:</b> You can add other values to nginx.conf to suit your needs.</li>
		<li><b> NGINX HTTP Section Custom Configuration:</b> You can add other values to nginx.conf in declaration of http {} to suit your needs.</li>
		<li><b> NGINX SERVER Section Custom Configuration:</b> You can add other values to nginx.conf in declaration of server {} to suit your needs.</li>
		<li><b> PHP Custom Configuration:</b> You can add other values to php.ini to suit your needs.</li>
		<li><b> Server Priority:</b> Sets the service priority over other processes running on the router.<br><br>
			The operating system kernel has priority -5.<br>
			Never select a lower value than the kernel uses. Do not use the service test page to adjust the server performance, 
			its performance is lower than the definitive media where files will be located, i.e; USB Stick, Hard Drive or SSD.</li>
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
