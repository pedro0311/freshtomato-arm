<!DOCTYPE html>
<!--
	Tomato GUI
	Media Server Settings - !!TB

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html lang="en-GB">
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] NAS: Media Server</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("ms_enable,ms_port,ms_dirs,ms_dbdir,ms_ifname,ms_tivo,ms_stdlna,ms_sas,cifs1,cifs2,jffs2_on,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

</script>
<script src="isup.jsx?_http_id=<% nv(http_id); %>"></script>

<script>
var up = new TomatoRefresh('isup.jsx', '', 5);

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

var mediatypes = [['','All Media Files'],['A','Audio only'],['V','Video only'],['P','Images only']];

function show() {
	var e = E('_media_button');
	e.value = (isup.minidlna ? 'Res' : 'S')+'tart Now';
	e.setAttribute('onclick', 'javascript:toggle(\'media\', '+isup.minidlna+');');
	e.disabled = !(nvram.ms_enable == '1' && E('_f_ms_enable').checked);

	elem.display('_media_button', nvram.ms_enable == '1');
}

function toggle(service, isup) {
	if (changed && !confirm("There are unsaved changes. Continue anyway?"))
		return;

	E('_'+service+'_button').disabled = 1;

	var fom = E('t_fom');
	fom._service.value = service+'-'+(isup ? 're' : '')+'start';
	save(1);
}

var msg = new TomatoGrid();

msg.setup = function() {
	this.init('ms-grid', 'sort', 50, [ { type: 'text', maxlen: 256 },{ type: 'select', options: mediatypes } ]);
	this.headerSet(['Directory', 'Content Filter']);

	var s = (''+nvram.ms_dirs).split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 2)
			this.insertData(-1, t);
	}

	this.sort(0);
	this.showNewEditor();
	this.resetNewEditor();
}

msg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	f[0].value = '';
	f[1].selectedIndex = 0;
}

msg.dataToView = function(data) {
	var b = [];
	var i;

	b.push(escapeHTML(''+data[0]));
	for (i = 0; i < mediatypes.length; ++i) {
		if (mediatypes[i][0] == (''+data[1])) {
			b.push(mediatypes[i][1]);
			break;
		}
	}
	if (b.length < 2)
		b.push(mediatypes[0][1]);

	return b;
}

msg.verifyFields = function(row, quiet) {
	var ok = 1;
	var f;
	f = fields.getAll(row);

	if (!v_nodelim(f[0], quiet, 'Directory', 1) || !v_path(f[0], quiet, 1))
		ok = 0;

	changed |= ok;

	return ok;
}

var xob = null;

function updateNotice() {
	if (xob)
		return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		if (text.length)
			text = '<div id="notice">'+text.replace(/\n/g, '<br>')+'<\/div><br style="clear:both">';

		elem.setInnerHTML('notice-msg', text);

		xob = null;
		setTimeout(updateNotice, 5000);
	}
	xob.onError = function(ex) { xob = null; }
	xob.post('update.cgi', 'exec=notice&arg0=dlna');
}

function verifyFields(focused, quiet) {
	var ok = 1;
	var a, b, v;
	var eLoc, eUser;

	var bridge1 = E('_ms_ifname');
	if (nvram.lan_ifname.length < 1)
		bridge1.options[0].disabled = 1;
	if (nvram.lan1_ifname.length < 1)
		bridge1.options[1].disabled = 1;
	if (nvram.lan2_ifname.length < 1)
		bridge1.options[2].disabled = 1;
	if (nvram.lan3_ifname.length < 1)
		bridge1.options[3].disabled = 1;

	a = E('_f_ms_enable').checked ? 1 : 0;

	eLoc = E('_f_loc');
	eUser = E('_f_user');

	eLoc.disabled = (a == 0);
	eUser.disabled = (a == 0);
	E('_ms_port').disabled = (a == 0);
	E('_ms_ifname').disabled = (a == 0);
	E('_f_ms_sas').disabled = (a == 0);
	E('_f_ms_rescan').disabled = (a == 0);
	E('_f_ms_tivo').disabled = (a == 0);
	E('_f_ms_stdlna').disabled = (a == 0);

	ferror.clear(eLoc);
	ferror.clear(eUser);

	v = eLoc.value;
	b = (v == '*user');
	elem.display(eUser, b);
	elem.display(PR('_f_ms_sas'), (v != ''));

	if (a == 0) {
		if (focused != E('_f_ms_rescan'))
			changed |= ok;
		return ok;
	}
	if (b) {
		if (!v_path(eUser, quiet || !ok, 1))
			ok = 0;
	}
/* JFFS2-BEGIN */
	else if (v == '/jffs/dlna') {
		if (nvram.jffs2_on != '1') {
			ferror.set(eLoc, 'JFFS is not enabled.', quiet || !ok);
			ok = 0;
		}
		else
			ferror.clear(eLoc);
	}
/* JFFS2-END */

	if (focused != E('_f_ms_rescan'))
		changed |= ok;

	return ok;
}

function save(nomsg) {
	if (msg.isEditing())
		return;
	if (!verifyFields(null, 0))
		return;

	var fom = E('t_fom');

	fom.ms_enable.value = fom._f_ms_enable.checked ? 1 : 0;
	nvram.ms_enable = fom._f_ms_enable.checked ? 1 : 0;
	fom.ms_tivo.value = fom._f_ms_tivo.checked ? 1 : 0;
	fom.ms_stdlna.value = fom._f_ms_stdlna.checked ? 1 : 0;
	fom.ms_rescan.value = fom._f_ms_rescan.checked ? 1 : 0;
	fom.ms_sas.value = fom._f_ms_sas.checked ? 1 : 0;

	var s = fom._f_loc.value;
	fom.ms_dbdir.value = (s == '*user') ? fom._f_user.value : s;

	var data = msg.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i)
		r.push(data[i].join('<'));

	fom.ms_dirs.value = r.join('>');
	fom._nofootermsg.value = (nomsg ? 1 : 0);
	fom._service.value = 'media-restart';

	form.submit(fom, 1);

	changed = 0;
	fom.f_ms_rescan.checked = 0;
}

function earlyInit() {
	show();
	msg.setup();
	verifyFields(null, 1);
}

function init() {
	changed = 0;
	up.initPage(250, 5);
	updateNotice();
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

<input type="hidden" name="_nextpage" value="nas-media.asp">
<input type="hidden" name="_service" value="">
<input type="hidden" name="_nofootermsg" value="">
<input type="hidden" name="ms_enable">
<input type="hidden" name="ms_dirs">
<input type="hidden" name="ms_dbdir">
<input type="hidden" name="ms_tivo">
<input type="hidden" name="ms_stdlna">
<input type="hidden" name="ms_rescan">
<input type="hidden" name="ms_sas">

<!-- / / / -->

<div class="section-title">Media / DLNA Server</div>
<div class="section">
	<script>
		switch (nvram.ms_dbdir) {
			case '':
			case '/jffs/dlna':
			case '/cifs1/dlna':
			case '/cifs2/dlna':
				loc = nvram.ms_dbdir;
			break;
			default:
				loc = '*user';
			break;
		}

		createFieldTable('', [
			{ title: 'Enable', name: 'f_ms_enable', type: 'checkbox', value: nvram.ms_enable == '1' },
			{ title: 'Listen on', indent: 2, name: 'ms_ifname', type: 'select', options: [['br0','LAN0 (br0)*'],['br1','LAN1 (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']], value: eval ('nvram.ms_ifname'), suffix: ' <small>* default<\/small> ' },
			{ title: 'Port', indent: 2, name: 'ms_port', type: 'text', maxlen: 5, size: 6, value: nvram.ms_port, suffix: '<small>(range: 0 - 65535; default (random) set 0)<\/small>' },
			{ title: 'Database Location', multi: [
				{ name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],
/* JFFS2-BEGIN */
					['/jffs/dlna','JFFS'],
/* JFFS2-END */
					['*user','Custom Path']], value: loc },
				{ name: 'f_user', type: 'text', maxlen: 256, size: 60, value: nvram.ms_dbdir }
			] },
			{ title: 'Scan Media at Startup*', indent: 2, name: 'f_ms_sas', type: 'checkbox', value: nvram.ms_sas == '1', hidden: 1 },
			{ title: 'Rescan on the next run*', indent: 2, name: 'f_ms_rescan', type: 'checkbox', value: 0, suffix: '<br><small>* Media scan may take considerable time to complete.<\/small>' },
			null,
			{ title: 'TiVo Support', name: 'f_ms_tivo', type: 'checkbox', value: nvram.ms_tivo == '1' },
			{ title: 'Strictly adhere to DLNA standards', name: 'f_ms_stdlna', type: 'checkbox', value: nvram.ms_stdlna == '1' }
		]);
	</script>
	<input type="button" value="" onclick="" id="_media_button">
</div>

<!-- / / / -->

<div id="notice-msg"></div>

<!-- / / / -->

<div class="section-title">Media Directories</div>
<div class="section">
	<div class="tomato-grid" id="ms-grid"></div>

	<small>To exclude a given subdirectory from the scan, place a file with the name <i>.minidlnaignore</i> in it.</small>
</div>

<!-- / / / -->

<div id="footer">
	<span id="footer-msg"></span>
	<input type="button" value="Save" id="save-button" onclick="save(0)">
	<input type="button" value="Cancel" id="cancel-button" onclick="reloadPage();">
</div>

</td></tr>
</table>
</form>
<script>earlyInit();</script>
</body>
</html>
