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
<title>[<% ident(); %>] IP Traffic: View Graphs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>
<script src="bwm-hist.js"></script>

<script>

//	<% nvram('cstats_enable,lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask,dhcpd_static,web_svg'); %>

//	<% iptraffic(); %>

/* REMOVE-BEGIN
	<% devlist(); %>
REMOVE-END */

nfmarks = [];
irates = [];
orates = [];

var svgReady = 0;
var abc = ['', '', '', '', '', '', '', '', '', '', ''];
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
	'ffd700'
];

/* REMOVE-BEGIN
var grid = new TomatoGrid();
REMOVE-END */

var lock = 0;

var prevtimestamp = new Date().getTime();
var thistimestamp;
var difftimestamp;
var avgiptraffic = [];
var lastiptraffic = iptraffic;
var cstats_busy = 0;

/* REMOVE-BEGIN
hostnamecache = [];
REMOVE-END */

function updateLabels() {
	var i = 0;
	while ((i < abc.length) && (i < iptraffic.length)) {
		abc[i] = iptraffic[i][0]; /* IP address */
		i++;
	}
}

updateLabels();

/* REMOVE-BEGIN
	var i = 0;
	while ((i <= nfmarks.length) && (i <= iptraffic.length)) {
	while (i < nfmarks.length) {
REMOVE-END */
	i = 0;
	while (i < 11) {
		if (iptraffic[i] != null) {
			nfmarks[i] = iptraffic[i][9] + iptraffic[i][10]; /* TCP + UDP connections */
		} else {
			nfmarks[i] = 0;
		}
		irates[i] = 0;
		orates[i] = 0;
		i++;
	}


function mClick(n) {
	location.href = 'ipt-details.asp?ipt_filterip=' + abc[n];
}

function showData() {
	var i, n, p;
	var ct, irt, ort;

	ct = irt = ort = 0;
	for (i = 0; i < 11; ++i) {
		if (!nfmarks[i]) nfmarks[i] = 0;
		ct += nfmarks[i];
		if (!irates[i]) irates[i] = 0;
		irt += irates[i];
		if (!orates[i]) orates[i] = 0;
		ort += orates[i];
	}

	for (i = 0; i < 11; ++i) {
		n = nfmarks[i];
		E('ccnt' + i).innerHTML = (abc[i] != '') ? n : '';
		if (ct > 0) p = (n / ct) * 100;
		else p = 0;
		E('cpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
	}
	E('ccnt-total').innerHTML = ct;

	for (i = 0; i < 11; ++i) {
		n = irates[i];
		E('bcnt' + i).innerHTML = (abc[i] != '') ? (n / 125).toFixed(2) : '';
		E('bcntx' + i).innerHTML = (abc[i] != '') ? (n / 1024).toFixed(2) : '';
		if (irt > 0) p = (n / irt) * 100;
		else p = 0;
		E('bpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
	}
	E('bcnt-total').innerHTML = (irt / 125).toFixed(2);
	E('bcntx-total').innerHTML = (irt / 1024).toFixed(2);

	for (i = 0; i < 11; ++i) {
		n = orates[i];
		E('obcnt' + i).innerHTML = (abc[i] != '') ? (n / 125).toFixed(2) : '';
		E('obcntx' + i).innerHTML = (abc[i] != '') ? (n / 1024).toFixed(2) : '';
		if (ort > 0) p = (n / ort) * 100;
		else p = 0;
		E('obpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
	}
	E('obcnt-total').innerHTML = (ort / 125).toFixed(2);
	E('obcntx-total').innerHTML = (ort / 1024).toFixed(2);
}

function getArrayPosByElement(haystack, needle, index) {
	for (var i = 0; i < haystack.length; ++i) {
		if (haystack[i][index] == needle) {
			return i;
		}
	}
	return -1;
}

var ref = new TomatoRefresh('update.cgi', 'exec=iptraffic', 2, 'ipt_graphs');

ref.refresh = function(text) {
	var b, i, j, k, l;

	++lock;

	thistimestamp = new Date().getTime();

	nfmarks = [];
	irates = [];
	orates = [];
	iptraffic = [];
	try {
		eval(text);
	}
	catch (ex) {
		nfmarks = [];
		irates = [];
		orates = [];
		iptraffic = [];
	}

	difftimestamp = thistimestamp - prevtimestamp;
	prevtimestamp = thistimestamp;

	for (i = 0; i < iptraffic.length; ++i) {
		b = iptraffic[i];

		j = getArrayPosByElement(avgiptraffic, b[0], 0);
		if (j == -1) {
			j = avgiptraffic.length;
			avgiptraffic[j] = [ b[0], 0, 0, 0, 0, 0, 0, 0, 0, b[9], b[10] ];
		}

		k = getArrayPosByElement(lastiptraffic, b[0], 0);

		if (k == -1) {
			k = lastiptraffic.length;
			lastiptraffic[k] = b;
		}

		for (l = 1; l <= 8; ++l) {
			avgiptraffic[j][l] = ((b[l] - lastiptraffic[k][l]) / difftimestamp * 1000);
			lastiptraffic[k][l] = b[l];
		}

		avgiptraffic[j][9] = b[9];
		avgiptraffic[j][10] = b[10];
		lastiptraffic[k][9] = b[9];
		lastiptraffic[k][10] = b[10];
	}
	-- lock;

/* REMOVE-BEGIN
	updateLabels();
REMOVE-END */

	i = 0;
	while (i < 11) {
		if (iptraffic[i] != null) {
			nfmarks[i] = avgiptraffic[i][9] + avgiptraffic[i][10]; /* TCP + UDP connections */
			irates[i] = avgiptraffic[i][1]; /* RX bytes */
			orates[i] = avgiptraffic[i][2]; /* TX bytes */
		} else {
			nfmarks[i] = 0;
			irates[i] = 0;
			orates[i] = 0;
		}
		++i;
	}

	showData();
	if (svgReady == 1) {
		updateCD(nfmarks, abc);
		updateBI(irates, abc);
		updateBO(orates, abc);
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
		updateBI(irates, abc);
		updateBO(orates, abc);
	}
	else if (--svgReady > -5) {
		setTimeout(checkSVG, 500);
	}
}

function init() {
	if (nvram.cstats_enable != '1') {
		E('refresh-time').setAttribute("disabled", "disabled");
		E('refresh-button').setAttribute("disabled", "disabled");
		return;
	}
	showData();
	checkSVG();
	ref.initPage(2000, 3);
	if (!ref.running) ref.once = 1;
	ref.start();
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

<div class="section-title">Connections Distribution (TCP/UDP)</div>

<div id="cstats" class="cstats">

	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					for (i = 0; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						  '<td class="bwm-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						  '<td class="title" style="width:45px"><a href="ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '<\/a><\/td>' +
						  '<td id="ccnt' + i + '" class="bwm-count" style="width:100px"><\/td>' +
						  '<td id="cpct' + i + '" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="ccnt-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="ipt-graph.svg?n=0" style="width:310px;height:310px" id="svg0" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Inbound)</div>
	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="bwm-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="bwm-thead bwm-count">kbit/s<\/td><td class="bwm-thead bwm-count">KB/s<\/td><td class="bwm-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 0; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="bwm-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						'<td class="title" style="width:45px"><a href="ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '<\/a><\/td>' +
						'<td id="bcnt' + i + '" class="bwm-count" style="width:60px"><\/td>' +
						'<td id="bcntx' + i + '" class="bwm-count" style="width:50px"><\/td>' +
						'<td id="bpct' + i + '" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="bcnt-total" class="bwm-total bwm-count"><\/td><td id="bcntx-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="ipt-graph.svg?n=1" style="width:310px;height:310px" id="svg1" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

<!-- / / / -->

	<div class="section-title">Bandwidth Distribution (Outbound)</div>
	<div class="section">
		<table class="bwm-svg">
			<tr><td>
				<script>
					W('<table style="width:270px">');
					W('<tr><td class="bwm-color"><\/td><td class="title" style="width:45px">&nbsp;<\/td><td class="bwm-thead bwm-count">kbit/s<\/td><td class="bwm-thead bwm-count">KB/s<\/td><td class="bwm-pct">&nbsp;');
					W('<\/td><\/tr>');
					for (i = 0; i < 11; ++i) {
						W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="bwm-color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;<\/td>' +
						'<td class="title" style="width:45px"><a href="ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '<\/a><\/td>' +
						'<td id="obcnt' + i + '" class="bwm-count" style="width:60px"><\/td>' +
						'<td id="obcntx' + i + '" class="bwm-count" style="width:50px"><\/td>' +
						'<td id="obpct' + i + '" class="bwm-pct"><\/td><\/tr>');
					}
					W('<tr><td>&nbsp;<\/td><td class="bwm-total">Total<\/td><td id="obcnt-total" class="bwm-total bwm-count"><\/td><td id="obcntx-total" class="bwm-total bwm-count"><\/td><td class="bwm-total bwm-pct">100%<\/td><\/tr>');
					W('<\/table>');
				</script>
			</td>
			<td>
				<script>
					if (nvram.web_svg != '0') {
						W('<embed src="ipt-graph.svg?n=2" style="width:310px;height:310px" id="svg2" type="image/svg+xml"><\/embed>');
					}
				</script>
			</td></tr>
		</table>
	</div>

</div>

<!-- / / / -->

<script>checkCstats();</script>

<!-- / / / -->

<div id="footer">
	<script>genStdRefresh(1,1,'ref.toggle()');</script>
</div>

</td></tr>
</table>
</form>
</body>
</html>
