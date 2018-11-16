<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: IPerf</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript" src="tomato.js"></script>

<!-- / / / -->

<style type="text/css">
#tp-grid .co1 {
	text-align: right;
	width: 30px;
}
#tp-grid .co2 {
	width: 440px;
}
#tp-grid .co3, #tp-grid .co4, #tp-grid .co5, #tp-grid .co6 {
	text-align: right;
	width: 70px;
}
#tp-grid .header .co1 {
	text-align: left;
}
</style>

<script type="text/javascript" src="debug.js"></script>

<script type="text/javascript">

//	<% nvram('lan_ipaddr'); %>	// http_id

iperf_up = parseInt('<% psup("iperf"); %>');

var ref = new TomatoRefresh('', '', 0, 'tools-shell_refresh');

ref.stop = function() {
	this.timer.start(1000);
}

ref.refresh = function(text) {
	execute();
}

function verifyFields(focused, quiet) {
	var s;
	var e;
	let transmitMode = E('iperf_transm').checked == true;
	let sizeLimitMode = E('iperf_time_limited').checked == true;

	if (transmitMode) {
		PR(E('iperf_addr')).style.display = '';
		PR(E('iperf_time_limited')).style.display = '';
		PR(E('iperf_proto_tcp')).style.display = '';
		if (sizeLimitMode) {
			PR('time_limit').style.display = '';
			PR('byte_limit').style.display = 'none';
		} else {
			PR('byte_limit').style.display = '';
			PR('time_limit').style.display = 'none';
		}
		E('notice1').style.display = 'none';
	} else {
		PR(E('iperf_addr')).style.display = 'none';
		PR(E('iperf_time_limited')).style.display = 'none';
		PR(E('iperf_proto_tcp')).style.display = 'none';
		PR(E('time_limit')).style.display = 'none';
		PR(E('byte_limit')).style.display = 'none';
		E('notice1').style.display = '';
	}
	toggleAllFields(!iperf_up);
	E('client_commandline_helper').innerHTML = generateClientHelperString();
}

function generateClientHelperString() {
	let helper = "iperf";
	helper += " -p ";
	helper += E('iperf_port').value;
	helper += (E('iperf_proto_udp').checked == true) ? " -u " : "";
	helper += " -c ";
	helper += nvram.lan_ipaddr;
	return helper;
}

function toggleAllFields(enable) {
	var rows = document.getElementsByClassName("fields")[0].rows;
	for (i=0; i < rows.length; i++) {
		let inputs = rows[i].getElementsByClassName("content")[0].children;
		for (j=0; j < inputs.length; j++) {
			inputs[j].disabled = !enable;
		}
	}
}

var cmd = null;

function spin(x) {
	if (!x) cmd = null;
}

function scaleSpeed(n) {
	if (isNaN(n *= 1)) return '-';
	var s = -1;
	do {
		n /= 1000;
		++s;
	} while ((n > 999) && (s < 2));
	return comma(n.toFixed(2)) + (' ') + (['Kbps', 'Mbps', 'Gbps'])[s];
}

function execute() {
	/* Opera 8 sometimes sends 2 clicks */
	if (cmd) return;
	spin(1);

	cmd = new XmlHttp();
	/* REMOVE-BEGIN
	// Sorry for this crazy logic but it's due iperf JSON output format
	// There are 3 important fields to be checked from JSON response
	// 1. error  - When this fields exists, then it means that iperf has been
	// closed unexpectedly, for ex. when connection is refused
	// 2. Subarray end - if this subarray is sent, then it means that test has
	// been completed successfully.
	// 3. sum - if there is a sum field, then it means that we got interval file.
	REMOVE-END */
	cmd.onCompleted = function(text, xml) {
		let respObj = JSON.parse(text);
		if (respObj.mode != undefined && (respObj.mode == 'Stopped'
				|| respObj.mode == 'Finished')) {
			iperf_up = 0;
		} else {
			iperf_up = 1;
		}
		if (respObj.error){ /* Error Happen, must be checked first */
			iperf_up = 0;
			E('test_status').innerHTML = respObj.error;
		} else {
			if (respObj.end) { /* Test finished */
				iperf_up = 0;
				E('test_status').innerHTML = 'Finished';
				if (respObj.start.accepted_connection) {
					E('test_xfered').innerHTML = scaleSize(respObj.end.sum_received.bytes) + ' <small>uploaded<\/small>';
					E('test_speed').innerHTML = scaleSpeed(respObj.end.sum_received.bits_per_second);
				} else {
					E('test_xfered').innerHTML = scaleSize(respObj.end.sum_sent.bytes) + ' <small>uploaded<\/small>';
					E('test_speed').innerHTML = scaleSpeed(respObj.end.sum_sent.bits_per_second);
				}
				E('test_time').innerHTML = respObj.end.sum_sent.seconds.toFixed(2) + ' <small>seconds<\/small>';
			} else { /* Interval has been send */
				E('test_status').innerHTML = respObj.mode;
				let sumSent = respObj.sum_sent;
				if (sumSent && sumSent.bytes != 0) {
					E('test_xfered').innerHTML = scaleSize(respObj.sum_sent.bytes) + ' <small>uploaded<\/small>';
				} else {
					if(respObj.sum_received && respObj.sum_received.bytes) {
						E('test_xfered').innerHTML = scaleSize(respObj.sum_received.bytes) + ' <small>uploaded<\/small>';
					}
				}
				if (respObj.sum) {
					E('test_time').innerHTML = respObj.sum.end.toFixed(2) + ' <small>seconds<\/small>';
					E('test_speed').innerHTML = scaleSpeed(respObj.sum.bits_per_second);
				}
			}
		}
		changeTestButtonText();
		toggleAllFields(!iperf_up);
		spin(0);
	}
	cmd.onError = function(x) {
		E('test_status').innerHTML = 'ERROR: ' + x;
		spin(0);
	}

	cmd.post('iperfstatus.cgi', '');
	setCookies();
}

function setCookies() {
	cookie.set('iperf_mode',  E('iperf_transm').checked == true);
	cookie.set('iperf_transmit_address',  E('iperf_addr').value);
	cookie.set('iperf_port',  E('iperf_port').value);
	cookie.set('iperf_time_limit_value',  E('time_limit').value);
	cookie.set('iperf_byte_limit_value',  E('byte_limit').value);
	cookie.set('iperf_time_limited',  E('iperf_time_limited').checked == true);
}

function init() {
	loadCookies();
	changeTestButtonText();
	verifyFields(1, 1);

	ref.start();
}

function loadCookies() {
	let s;
	if ((s = cookie.get('iperf_mode')) != null) {
		 E('iperf_transm').checked = s == "true";
		 E('iperf_recv').checked = s != "true";
	}
	if ((s = cookie.get('iperf_transmit_address')) != null) E('iperf_addr').value = s;
	if ((s = cookie.get('iperf_port')) != null) E('iperf_port').value = s;
	if ((s = cookie.get('iperf_time_limit_value')) != null) E('time_limit').value = s;
	if ((s = cookie.get('iperf_byte_limit_value')) != null) E('byte_limit').value = s;
	if ((s = cookie.get('iperf_time_limited')) != null) {
		if (s == '0') {
			E('time_limit').checked = false;
			E('byte_limit').checked = true;
		} else {
			E('time_limit').checked = true;
			E('byte_limit').checked = false;
		}
	}
	if ((s = cookie.get('iperf_mode')) != null) {
		if (s == '0') {
			E('iperf_transm').checked = false;
			E('iperf_recv').checked = true;
		} else {
			E('iperf_transm').checked = true;
			E('iperf_recv').checked = false;
		}
	}
}

function changeTestButtonText() {
	E('runtestbutton').value = (iperf_up == 1) ? 'Stop test' : 'Start test';
}

function runButtonClick() {
	let requestCommand = new XmlHttp();
	requestCommand.onCompleted = function(text, xml) {
		execute();
	}
	requestCommand.onError = function(x) {
		E('test_status').innerHTML = 'ERROR: ' + x;
		execute();
	}
	if (iperf_up == 1) {
		requestCommand.post('iperfkill.cgi','');
	} else {
		let transmitMode = E('iperf_transm').checked == true;
		let limitMode = E('iperf_size_limited').checked == true;
		let limit = E(limitMode ? 'byte_limit' : 'time_limit').value;
		let udpProtocol = E('iperf_proto_udp').checked == true;
		let ttcpPort = E('iperf_port').value;
		let paramStr = '_mode=' + (transmitMode ? 'client' : 'server')
			+ '&_udpProto=' + (udpProtocol ? '1' : '0')
			+ '&_port=' + ttcpPort
			+ '&_limitMode=' + (limitMode ? '1' : '0')
			+ '&_limit=' + limit;
		if (transmitMode) {
			paramStr += '&_host=' + E('iperf_addr').value;
		}
		requestCommand.post('iperfrun.cgi', paramStr);
	}
	E('test_status').innerHTML = '';
	E('test_xfered').innerHTML = '';
	E('test_time').innerHTML = '';
	E('test_speed').innerHTML = '';
}
</script>

</head>
<body onload="init()">
<form action="javascript:{}">
<table id="container" cellspacing="0">
<tr><td colspan="2" id="header">
	<div class="title">Tomato</div>
	<div class="version">Version <% version(); %></div>
</td></tr>
<tr id="body"><td id="navi"><script type="text/javascript">navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div class="section-title">Bandwidth benchmark</div>
<div class="section">
<script type="text/javascript">
createFieldTable('', [
	{ title: 'Mode', multi: [
		{suffix: '&nbsp; Server&nbsp;&nbsp;&nbsp;', name: 'test_mode', id: 'iperf_recv', type: 'radio', value: true },
		{suffix: '&nbsp; Client&nbsp;', name: 'test_mode', id: 'iperf_transm', type: 'radio'  } ]},
	{ title: 'Host address', name: 'f_addr', id: 'iperf_addr', maxlen: 50, type: 'text' },
	{ title: 'Protocol', multi: [
		{ suffix: '&nbsp; TCP&nbsp;&nbsp;&nbsp;', name: 'test_proto', id: 'iperf_proto_tcp', type: 'radio',  value: true },
		{ suffix: '&nbsp; UDP&nbsp;', name: 'test_proto', id: 'iperf_proto_udp', type: 'radio' } ]},
	{ title: 'Port', name: 'f_port', type: 'text', id: 'iperf_port', maxlen: 5, size: 5, value: '5201' },
	{ title: 'Type', id: 'type_selector', multi: [
		{suffix: '&nbsp; Time-limited&nbsp;&nbsp;&nbsp;', name: 'test_type', id: 'iperf_time_limited', type: 'radio', value: true },
		{suffix: '&nbsp; Buffer-limited&nbsp;', name: 'test_type', id: 'iperf_size_limited', type: 'radio'  } ]},
	{ title: 'Time limit', name: 'f_time_limit', id: 'time_limit', maxlen: 20, type: 'text', suffix: ' <small>seconds<\/small>', value: '10' },
	{ title: 'Bytes limit', name: 'f_bytes_limit', id: 'byte_limit', maxlen: 40, type: 'text', suffix: ' <small>bytes<\/small>', value: '1024' }
]);
W('<input type="button" value="Start test" onclick="runButtonClick()" id="runtestbutton">');
</script>
</div>

<div id="notice1">Please use following command on another network node to start test:
<br /><br />
<div id="client_commandline_helper" style="text-align:center;"><i><b>iperf -J -c </b></i></div>
<br />
IPerf in version 3 is required.
</div><br style="clear:both">

<div id="status_table">
<script type="text/javascript">
createFieldTable('', [
	{ title: 'Status', text: '<div id="test_status"><\/div>' },
	{ title: 'Test time', text: '<div id="test_time"><\/div>' },
	{ title: 'Transferred bytes', text: '<div id="test_xfered"><\/div>' },
	{ title: 'Speed', text: '<div id="test_speed"><\/div>' }
]);
</script>
<!--<script type="text/javascript">genStdRefresh(1,1,'ref.toggle()');</script>-->
</div>

<div style="height:10px;" onclick='E("debug").style.display=""'></div>
<textarea id="debug" style="width:99%;height:300px;display:none" cols="50" rows="10"></textarea>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">&nbsp;</td></tr>
</table>
</form>
</body>
</html>
