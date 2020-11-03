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
<title>[<% ident(); %>] QoS: View Graphs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("qos_classnames,web_svg,qos_enable,wan_qos_obw,wan_qos_ibw,wan2_qos_obw,wan2_qos_ibw,wan3_qos_obw,wan3_qos_ibw,wan4_qos_obw,wan4_qos_ibw"); %>

//	<% qrate(); %>

qrates_out = [0,0,0,0,0,0,0,0,0,0,0];
qrates_in = [0,0,0,0,0,0,0,0,0,0,0];
for (var i = 1; i < 11; i++) {
/* DUALWAN-BEGIN */
	qrates_in[i] = qrates1_in[i]+qrates2_in[i];
	qrates_out[i] = qrates1_out[i]+qrates2_out[i];
/* DUALWAN-END */
/* MULTIWAN-BEGIN */
	qrates_in[i] = qrates1_in[i]+qrates2_in[i]+qrates3_in[i]+qrates4_in[i];
	qrates_out[i] = qrates1_out[i]+qrates2_out[i]+qrates3_out[i]+qrates4_out[i];
/* MULTIWAN-END */
}

var svgReady = 0;

var Unclassified = ['Unclassified'];
var Unused = ['Unused'];
var classNames = nvram.qos_classnames.split(' ');
var abc = Unclassified.concat(classNames,Unused);

var colors = [
	'c6e2ff',
	'b0c4de',
	'9ACD32',
	'3cb371',
	'6495ed',
	'8FBC8F',
	'a0522d',
	'deb887',
	'F08080',
	'ffa500',
	'ffd700',
	'D8D8D8'
];

var toggle = true;

function mClick(n) {
	location.href = 'qos-detailed.asp?class=' + n;
}

function showData() {
	var i, n, p;
	var ct, irt, ort;

	ct = irt = ort = 0;

	for (i = 0; i < 11; ++i) {
		if (!nfmarks[i]) nfmarks[i] = 0;
		ct += nfmarks[i];
		if (!qrates_in[i]) qrates_in[i] = 0;
		irt += qrates_in[i];
		if (!qrates_out[i]) qrates_out[i] = 0;
		ort += qrates_out[i];
	}

	for (i = 0; i < 11; ++i) {
		n = nfmarks[i];
		E('ccnt' + i).innerHTML = n;
		if (ct > 0) p = (n / ct) * 100;
		else p = 0;
		E('cpct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('ccnt-total').innerHTML = ct;

	ibwrate = nvram.qos_ibw * 1000;
	obwrate = nvram.qos_obw * 1000;

	if (toggle == false) {
		totalirate = irt;
		totalorate = ort;
		totalratein = '100%';
		totalrateout = '100%';
	}
	else {
		FreeIncoming = (ibwrate - irt);
		qrates_in.push(FreeIncoming);
		FreeOutgoing = (obwrate - ort);
		qrates_out.push(FreeOutgoing);
		totalirate = ibwrate;
		totalorate = obwrate;
		totalratein = ((irt / totalirate) * 100).toFixed(2) + '%';
		totalrateout = ((ort / totalorate) * 100).toFixed(2) + '%';
	}

	for (i = 1; i < 11; ++i) {
		n = qrates_in[i];
		E('bicnt' + i).innerHTML = (n / 1000).toFixed(2)
		E('bicntx' + i).innerHTML = (n / 8192).toFixed(2)
		if (irt > 0)
			p = (n / totalirate) * 100;
		else
			p = 0;

		E('bipct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('bicnt-total').innerHTML = (irt / 1000).toFixed(2)
	E('bicntx-total').innerHTML = (irt / 8192).toFixed(2)
	E('ratein').innerHTML = totalratein;

	for (i = 1; i < 11; ++i) {
		n = qrates_out[i];
		E('bocnt' + i).innerHTML = (n / 1000).toFixed(2)
		E('bocntx' + i).innerHTML = (n / 8192).toFixed(2)
		if (ort > 0)
			p = (n / totalorate) * 100;
		else
			p = 0;

		E('bopct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('bocnt-total').innerHTML = (ort / 1000).toFixed(2)
	E('bocntx-total').innerHTML = (ort / 8192).toFixed(2)
	E('rateout').innerHTML = totalrateout;
}

var ref = new TomatoRefresh('update.cgi', 'exec=qrate', 2, 'qos_graphs');

ref.refresh = function(text) {
	nfmarks = [];
	qrates_in = [];
	qrates_out = [];

	for (var i = 1; i < 11; i++) {
/* DUALWAN-BEGIN */
		qrates_in[i] = qrates1_in[i]+qrates2_in[i];
		qrates_out[i] = qrates1_out[i]+qrates2_out[i];
/* DUALWAN-END */
/* MULTIWAN-BEGIN */
		qrates_in[i] = qrates1_in[i]+qrates2_in[i]+qrates3_in[i]+qrates4_in[i];
		qrates_out[i] = qrates1_out[i]+qrates2_out[i]+qrates3_out[i]+qrates4_out[i];
/* MULTIWAN-END */
	}
	try {
		eval(text);
	}
	catch (ex) {
		nfmarks = [];
		qrates_in = [];
		qrates_out = [];
	}

	showData();
	if (svgReady == 1) {
		updateCD(nfmarks, abc);
		updateBI(qrates_in, abc);
		updateBO(qrates_out, abc);
	}
}

function checkSVG() {
	var i, e, d, w;

	try {
		for (i = 2; i >= 0; --i) {
			e = E('svg' + i);
			d = e.getSVGDocument();

			if (d.defaultView) {
				w = d.defaultView;
			}
			else {
				w = e.getWindow();
			}

			if (!w.ready) break;

			switch (i) {
				case 0: {
					updateCD = w.updateSVG;
					break;
				}
				case 1: {
					updateBI = w.updateSVG;
					break;
				}
				case 2: {
					updateBO = w.updateSVG;
					break;
				}
			}
		}
	}
	catch (ex) {
	}

	if (i < 0) {
		svgReady = 1;
		updateCD(nfmarks, abc);
		updateBI(qrates_in, abc);
		updateBO(qrates_out, abc);
	}
	else if (--svgReady > -5) {
		setTimeout(checkSVG, 500);
	}
}

function showGraph() {
	if (toggle == true) {
		toggle = false;
		qrates_in = qrates_in.slice(0, -1);
		qrates_out = qrates_out.slice(0, -1);
		showData();
		checkSVG();
	}
	else {
		toggle = true;
		showData();
		checkSVG();
	}
}

function init() {
	if (nvram.qos_enable != '1') {
		E('qosstats').style.display = 'none';
		E('qosstatsoff').style.display = 'block';
		E('note-disabled').style.display = 'block';
		E('refresh-time').setAttribute("disabled", "disabled");
		E('refresh-button').setAttribute("disabled", "disabled");
		return;
	}

	nbase = fixInt(cookie.get('qnbase'), 0, 1, 0);
	showData();
	checkSVG();
	showGraph();
	ref.initPage(2000, 3);
}
</script>
</head>

<body onload="init()">
<form id="t_fom" action="javascript:{}">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div class="section-title" id="qosstatsoff" style="display:none">View Graphs</div>
<div id="qosstats">
	<div class="section-title">Connections Distribution</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					for (i = 0; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="qos-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						'<td class="title" style="width:45px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '<\/a><\/td>' +
						'<td id="ccnt' + i + '" class="qos-count" style="width:100px"><\/td>' +
						'<td id="cpct' + i + '" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="ccnt-total" class="qos-total qos-count"><\/td><td class="qos-total qos-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="qos-graph.svg?n=0" style="width:310px;height:310px" id="svg0" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Inbound)</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="qos-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="qos-thead qos-count">kbit/s<\/td><td class="qos-thead qos-count">KB/s<\/td><td class="qos-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 1; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="qos-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						'<td class="title" style="width:45px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '<\/a><\/td>' +
						'<td id="bicnt' + i + '" class="qos-count" style="width:60px"><\/td>' +
						'<td id="bicntx' + i + '" class="qos-count" style="width:50px"><\/td>' +
						'<td id="bipct' + i + '" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="bicnt-total" class="qos-total qos-count"><\/td><td id="bicntx-total" class="qos-total qos-count"><\/td><td id="ratein" class="qos-total qos-pct"><\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="qos-graph.svg?n=1" style="width:310px;height:310px" id="svg1" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Outbound)</div>
	<div class="section">
		<table class="qos-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="qos-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="qos-thead qos-count">kbit/s<\/td><td class="qos-thead qos-count">KB/s<\/td><td class="qos-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 1; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="qos-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						'<td class="title" style="width:45px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '<\/a><\/td>' +
						'<td id="bocnt' + i + '" class="qos-count" style="width:60px"><\/td>' +
						'<td id="bocntx' + i + '" class="qos-count" style="width:50px"><\/td>' +
						'<td id="bopct' + i + '" class="qos-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="qos-total">Total<\/td><td id="bocnt-total" class="qos-total qos-count"><\/td><td id="bocntx-total" class="qos-total qos-count"><\/td><td id="rateout" class="qos-total qos-pct"><\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="qos-graph.svg?n=2" style="width:310px;height:310px" id="svg2" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>QoS disabled.</b><br><br><a href="qos-settings.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,2,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
</body>
</html>
