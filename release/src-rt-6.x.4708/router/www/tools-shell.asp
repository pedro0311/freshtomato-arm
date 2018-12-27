<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<!--
	Tomato GUI

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Tools: System Commands</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script type="text/javascript" src="tomato.js"></script>
<script type="text/javascript" src="termlib_min.js"></script>

<!-- / / / -->

<style type="text/css">

#icommandsFromMobile {
	font-size: 32px;
	width: 512px;
}

.helperButton{
	font-size: 32px;
	height:64px;
	width:259px;
}
.lh15 {
	line-height: 15px;
}

.term {
	font-family: "Courier New",courier,fixed,monospace;
	font-size: 12px;
	color: #d3d3d3;
	background: none;
	letter-spacing: 1px;
}
.term .termReverse {
	color: #232e45;
	background: #95a9d5;
}
</style>

<script type="text/javascript" src="debug.js"></script>
<script type="text/javascript">

//	<% nvram(''); %>	// http_id

var term;
var working_dir = '/www';

function verifyFields(focused, quiet) {
	return 1;
}

function termOpen() {
	if ((!term) || (term.closed)) {
		term = new Terminal(
			{
				cols: 94,
				rows: 35,
				mapANSI: true,
				crsrBlinkMode: true,
				closeOnESC: false,
				termDiv: 'termDiv',
				handler: termHandler,
				initHandler: initHandler,
				wrapping: true
			}
		);
		term.open();
	}
	window.addEventListener("paste", function(thePasteEvent) {
		if (term) {
			let clipboardData, pastedData;
			thePasteEvent.stopPropagation();
			thePasteEvent.preventDefault();
			clipboardData = thePasteEvent.clipboardData || window.clipboardData;
			pastedData = clipboardData.getData('Text');
			term.type(pastedData);
		}
	}, false);
}

function initHandler() {
	term.write('%+r FreshTomato Web Shell ready. %-r \n\n');
	runCommand('mymotd');
}

function showWait(state) {
	E('wait').style.visibility = state ? 'visible' : 'hidden';
}

function termHandler() {
	this.newLine();

	this.lineBuffer = this.lineBuffer.replace(/^\s+/, '');
	if (this.lineBuffer.startsWith("cd ")) {
		let tmp = this.lineBuffer.slice(3);

		if (tmp === ".") {
			return; /* nothing to do here.. */
		} else if (tmp === "..") {
			workingDirUp();
			term.write('%+r Switching to directory %+b' + working_dir + '%-b %-r \n');
			term.prompt();
		} else {
			checkDirectoryExist(tmp);
		}
	} else if (this.lineBuffer === "clear") {
		term.clear();
		term.prompt();
	} else {
		runCommand(this.lineBuffer);
	}
	return;
}

function checkDirectoryExist(directoryToCheck) {
	let cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		if (text.trim() === "OK") {
			if (!directoryToCheck.startsWith("/")) {
				working_dir = working_dir + "/" + directoryToCheck;
			} else {
				working_dir = directoryToCheck;
			}
			term.write('%+r Switching to directory %+b' + working_dir + '%-b %-r \n');
		} else {
			term.write( '%+r%c(red) ERROR directory ' + directoryToCheck + ' does not exist %c(default)%-r' );
		}
		showWait(false);
		term.prompt();
	}
	cmd.onError = function(text) {
		term.write( '%c(red) ERROR during switching directory: ' + text + ' %c(default)' );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	if (!directoryToCheck.startsWith("/")) {
		directoryToCheck = working_dir + "/" + directoryToCheck;
	}
	cmd.post('shell.cgi', 'action=execute&nojs=1&command=' + escapeCGI("[ -d \"" + directoryToCheck + "\" ] && echo \"OK\""));
}

function workingDirUp() {
	let tmp = working_dir.split("/");
	if (tmp.length > 1) {
		tmp.pop();
		working_dir = tmp.join("/");
		if (working_dir === "") { /* it was last dir; */
			working_dir = "/";
		}
	}
}

function runCommand(command) {
	let cmd = new XmlHttp();
	cmd.onCompleted = function(text, xml) {
		term.write(text.split('\n'), true);
		showWait(false);
	}
	cmd.onError = function(text) {
		term.type( '> ERROR: ' + text, 17 );
		showWait(false);
		term.prompt();
	}

	showWait(true);
	cmd.post('shell.cgi', 'action=execute&nojs=1&working_dir=' + working_dir + '&command=' + escapeCGI(command));
}

function fakecommand() {
	let command = E('icommandsFromMobile').value;
	for (var i=0; i<command.length; i++) {
		Terminal.prototype.globals.keyHandler({which: command.charCodeAt(i), _remapped:true});
	}
	sendCR();
	E('icommandsFromMobile').value = '';
	E('icommandsFromMobile').focus();
}

function sendSpace() {
	Terminal.prototype.globals.keyHandler({which: 32, _remapped:false});
	sendCR();
}

function sendCR() {
	Terminal.prototype.globals.keyHandler({which: 0x0D, _remapped:false});
}

function toggleHWKeyHelper() {
	E('noHWKeyHelperDiv').style.visibility = 'visible';
	E('noHWKeyHelperLink').style.visibility = 'hidden';
}

</script>

</head>

<body onload="termOpen()">
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

<div class="section-title">Execute System Commands</div>
<div class="section">
<div id="termDiv"></div>
</div>
<a href="#" onclick="toggleHWKeyHelper();" id="noHWKeyHelperLink">No hardware keyboard</a>
<div style="visibility:hidden;text-align:left" id="noHWKeyHelperDiv">
	<input type="button" class="helperButton" onclick="fakecommand()" value="Enter">
	<input type="button" class="helperButton" onclick="sendSpace()" value="Space"><br>
	<input type="text" id="icommandsFromMobile" name="commandsFromMobile" spellcheck="false" style="text-transform:none">
</div>
<div style="visibility:hidden;text-align:right" id="wait">Please wait... <img src="spin.gif" alt="" style="vertical-align:top"></div>
<pre id="result"></pre>

<!-- / / / -->

</td></tr>
<tr><td id="footer" colspan="2">&nbsp;</td></tr>
</table>
</form>
</body>
</html>