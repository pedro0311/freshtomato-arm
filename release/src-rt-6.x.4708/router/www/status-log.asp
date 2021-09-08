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
<title>[<% ident(); %>] Status: Logs</title>
<link rel="stylesheet" type="text/css" href="tomato.css">
<% css(); %>
<script src="tomato.js"></script>

<script>

//	<% nvram("log_file"); %>

var cprefix = 'status_log';

var currentSearch = '';
var currentFilterValue = 0;
var currentlyScrolling = false;
var scrollingDetectorTimeout;
var messages;
var entriesMode = 0;
var entriesLast = -1;

var LINE_PARSE_MAP_DATE_POS = 0;
var LINE_PARSE_MAP_FACILITY_POS = 1;
var LINE_PARSE_MAP_LEVEL_POS = 2;
var LINE_PARSE_MAP_LEVEL_ATTR_POS = 3;
var LINE_PARSE_MAP_LEVEL_PROCESS_POS = 4;
var LINE_PARSE_MAP_LEVEL_MESSAGE_POS = 5;

var ref = new TomatoRefresh('update.cgi', 'exec=showlog', 5, 'status_log');

ref.refresh = function(text) {
	try {
		messages = text.split('\n');
		if (E('log-find-text').value.length == 0) {
			if (!currentlyScrolling) {
				var willScroll = false;
				var tableDiv = E('log-table');
				if (tableDiv.offsetHeight + tableDiv.scrollTop >= tableDiv.scrollHeight)
					willScroll = true;

				logGrid.populate();

				if (willScroll)
					scrollToBottom();
			}
		}
	}
	catch (ex) {
	}
}

var logHeaderGrid = new TomatoGrid();

logHeaderGrid.setup = function() {
	this.init('log-table-header');
	this.headerSet(['Date','Facility','Level','Process','Message']);
}

var logGrid = new TomatoGrid();

logGrid.setup = function() {
	this.init('log-table', '', 4000);
	this.canSort = false;
	this.canEdit = false;
	this.canMove = false;
	this.canDelete = false
	this.populate();
}

logGrid.populate = function() {
	this.removeAllData();
	if (messages != null) {
		var messagesToAdd = messages;
		if (entriesMode != 0)
			messagesToAdd = messagesToAdd.slice(-1 * entriesMode);

		if (currentSearch)
			messagesToAdd = messagesToAdd.filter( function(line) { if (line != 'undefined') { return line.indexOf(currentSearch) >= 0; } } );

		var count = 0;
		for (var index = 0; index < messagesToAdd.length; ++index) {
			if (messagesToAdd[index]) {
				var logLineMap = getLogLineParsedMap(messagesToAdd[index]);
				if ((currentFilterValue == 0) || (logLineMap[LINE_PARSE_MAP_LEVEL_ATTR_POS][1] == currentFilterValue)) {
						if (!currentSearch || containsSearch(logLineMap, currentSearch)) {
							var row = createHighlightedRow(logLineMap);
							this.insert(-1, row, row, true);
							count++;
					}
				}
			}
		}

		var occurenceSpan = E('log-occurence-span');
		occurenceSpan.style.visibility = (currentSearch ? 'visible' : 'hidden');
		var occurenceValue = E('log-occurence-value');
		occurenceValue.innerHTML = count;

		var e = E('log-table').children[0].children[0].children[0];
		if (e) {
			var d = E('log-table-header').children[0].children[0].children[0];
			d.children[0].style.width = getComputedStyle(e.children[0]).width;
			d.children[1].style.width = getComputedStyle(e.children[1]).width;
			d.children[2].style.width = getComputedStyle(e.children[2]).width;
			d.children[3].style.width = getComputedStyle(e.children[3]).width;
		}
	}
}

logGrid.onClick = function(cell) {
	copyRowContent(PR(cell));
	var e = E('footer-msg2');
	e.style.display = 'inline-block';
	setTimeout(
		function() {
			e.style.display = 'none';
		}, 5000);
}

function copyRowContent(el) {
	var body = document.body, range, sel;
	if (document.createRange && window.getSelection) {
		range = document.createRange();
		sel = window.getSelection();
		sel.removeAllRanges();
		try {
			range.selectNodeContents(el);
			sel.addRange(range);
		} catch (e) {
			range.selectNode(el);
			sel.addRange(range);
		}
		document.execCommand('copy');
		sel.removeAllRanges();
	}
	else if (body.createTextRange) {
		range = body.createTextRange();
		range.moveToElementText(el);
		range.select();
		range.execCommand('Copy');
	}
}

var logLineRegex = new RegExp(/(\w+\s+\d+\s\d+\:\d+\:\d+)\s\w+\s(\w+).(\w+)\s(\S+)\s(.*)/mi);
var errRegex = new RegExp(/^(.*?)err.*/i);
var infRegex = new RegExp(/^(.*?)inf.*/i);
var noticeRegex = new RegExp(/^(.*?)notic.*/i);
var warnRegex = new RegExp(/^(.*?)war.*/i);
var alertRegex = new RegExp(/^(.*?)aler.*/i);
var criticalRegex = new RegExp(/^(.*?)crit.*/i);
var emergencyRegex = new RegExp(/^(.*?)emer.*/i);
var debugRegex = new RegExp(/^(.*?)debu.*/i);

function containsSearch(logLineMap, text) {
	return (String(logLineMap[LINE_PARSE_MAP_DATE_POS]).indexOf(text) >= 0 ||
	        String(logLineMap[LINE_PARSE_MAP_FACILITY_POS]).indexOf(text) >= 0 ||
	        String(logLineMap[LINE_PARSE_MAP_LEVEL_PROCESS_POS]).indexOf(text) >= 0 ||
	        String(logLineMap[LINE_PARSE_MAP_LEVEL_MESSAGE_POS]).indexOf(text) >= 0);
}

function getLevelColor(level) {
	if (emergencyRegex.test(level)) {
		this.levelColor = 'loglevel-1';
		this.levelNum = 1;
	}
	else if (alertRegex.test(level)) {
		this.levelColor = 'loglevel-2';
		this.levelNum = 2;
	}
	else if (criticalRegex.test(level)) {
		this.levelColor = 'loglevel-3';
		this.levelNum = 3;
	}
	else if (errRegex.test(level)) {
		this.levelColor = 'loglevel-4';
		this.levelNum = 4;
	}
	else if (warnRegex.test(level)) {
		this.levelColor = 'loglevel-5';
		this.levelNum = 5;
	}
	else if (noticeRegex.test(level)) {
		this.levelColor = 'loglevel-6';
		this.levelNum = 6;
	}
	else if (infRegex.test(level)) {
		this.levelColor = 'loglevel-7';
		this.levelNum = 7;
	}
	else if (debugRegex.test(level)) {
		this.levelColor = 'loglevel-8';
		this.levelNum = 8;
	}

	return [this.levelColor, this.levelNum];
}

function getLogLineParsedMap(logLine) {
	var returnedArray = [];
	var matchedArray = logLine.match(logLineRegex);
	if (matchedArray != null) {
		returnedArray[LINE_PARSE_MAP_DATE_POS] = matchedArray[1];
		returnedArray[LINE_PARSE_MAP_FACILITY_POS] = matchedArray[2];
		returnedArray[LINE_PARSE_MAP_LEVEL_POS] = matchedArray[3];
		returnedArray[LINE_PARSE_MAP_LEVEL_ATTR_POS] = getLevelColor(returnedArray[LINE_PARSE_MAP_LEVEL_POS]);
		returnedArray[LINE_PARSE_MAP_LEVEL_PROCESS_POS] = matchedArray[4].slice(0, -1);
		returnedArray[LINE_PARSE_MAP_LEVEL_MESSAGE_POS] = escapeHTML(matchedArray[5]);
	}
	return returnedArray;
}

function createHighlightedRow(logLineMap) {
	return [ generateHighlightSpan(logLineMap[LINE_PARSE_MAP_DATE_POS], 'co1', null),
	         generateHighlightSpan(logLineMap[LINE_PARSE_MAP_FACILITY_POS], 'co2', null),
	         generateHighlightSpan(logLineMap[LINE_PARSE_MAP_LEVEL_POS], 'co3', logLineMap[LINE_PARSE_MAP_LEVEL_ATTR_POS][0]),
	         generateHighlightSpan(logLineMap[LINE_PARSE_MAP_LEVEL_PROCESS_POS], 'co4', null),
	         generateHighlightSpan(logLineMap[LINE_PARSE_MAP_LEVEL_MESSAGE_POS], 'co5', null)
	];
}

function generateHighlightSpan(innerText, classN, customStyle) {
	var newText = document.createElement('td');
	newText.className = classN;
	if (customStyle)
		newText.className += ' '+customStyle;

	var indexOfSearch = innerText.indexOf(currentSearch);
	if (indexOfSearch == -1)
		newText.innerHTML = innerText;
	else {
		var sizeOfSearch = currentSearch.length;

		var stringBeforeFound = '';
		if (indexOfSearch != 0) {
			stringBeforeFound = innerText.substring(0, indexOfSearch);
			newText.innerHTML += stringBeforeFound;
		}

		var highlightedString = innerText.substring(indexOfSearch, indexOfSearch + sizeOfSearch);
		if (highlightedString)
			newText.innerHTML += '<span style="background-color:yellow">'+highlightedString+'<\/span>';

		var stringAfterFound = '';
		if (indexOfSearch + sizeOfSearch < innerText.length) {
			stringAfterFound = innerText.substring(indexOfSearch + sizeOfSearch, innerText.length);
			newText.innerHTML += stringAfterFound;
		}
	}

	return newText;
}

function filterLevelChanged() {
	var filterSelector = E('filterLevelSelector');
	currentFilterValue = parseInt(filterSelector.options[filterSelector.selectedIndex].value, 10);
	logGrid.populate();
	scrollToBottom();
}

function scrollToBottom() {
	var objDiv = E('log-table');
	objDiv.scrollTop = objDiv.scrollHeight;
}

function showEntries() {
	if (entriesMode == entriesLast)
		return;

	var e;
	elem.removeClass('entries'+entriesLast, 'selected');
	if ((e = E('entries'+entriesMode)) != null) {
		elem.addClass(e, 'selected');
		e.blur();
	}
	entriesLast = entriesMode;
}

function viewLast(n) {
	entriesMode = n;
	logGrid.populate();
	scrollToBottom();
	showEntries();
	cookie.set(cprefix+'_entries', entriesMode);
}

function onTableScroll() {
	/* Detector for smooth scrolling */
	if (scrollingDetectorTimeout)
		clearTimeout(scrollingDetectorTimeout);

	currentlyScrolling = true;
	scrollingDetectorTimeout = setTimeout(
		function() {
			currentlyScrolling = false;
		},
	150);
}

function onKeyUpEvent(event) {
	if (event.defaultPrevented)
		return;

	var key = event.key || event.keyCode;
	if (key === 'Escape' || key === 'Esc' || key === 27) {
		var findTextInput = E('log-find-text');
		findTextInput.value = '';
		currentSearch = '';
		var occurenceSpan = E('log-occurence-span');
		occurenceSpan.style.visibility = 'hidden';
		logGrid.populate();
		scrollToBottom();
	}
}

function init() {
	if (nvram.log_file != 1) {
		E('logging').style.display = 'none';
		E('note-disabled').style.display = 'block';
		return;
	}

	var c;
	if (((c = cookie.get(cprefix+'_entries')) != null) && (c >= '0'))
		entriesMode = (cookie.get(cprefix+'_entries'));
	showEntries();

	logHeaderGrid.setup();
	logGrid.setup();

	var findTextInput = E('log-find-text');
	addEvent(findTextInput, 'input', function(event) {
		setTimeout(function() {
			currentSearch = findTextInput.value;
			logGrid.populate();
			if (currentSearch.length == 0) {
				var occurenceSpan = E('log-occurence-span');
				occurenceSpan.style.visibility = 'hidden';
				scrollToBottom();
			}
		}, 1500);
	});
	addEvent(document, 'keyup', onKeyUpEvent);

	ref.initPage(0, 1);
	if (!ref.running)
		ref.once = 1;

	ref.start();
}

</script>
</head>

<body onload="init()">
<form id="t_fom">
<table id="container">
<tr><td colspan="2" id="header">
	<div class="title">FreshTomato</div>
	<div class="version">Version <% version(); %> on <% nv("t_model_name"); %></div>
</td></tr>
<tr id="body"><td id="navi"><script>navi()</script></td>
<td id="content">
<div id="ident"><% ident(); %></div>

<!-- / / / -->

<div class="section-title">Logs</div>
<div id="logging">
	<div class="section">
		<div class="log-searchbox">

			<span class="log-clear">
				&raquo; <a href="javascript:scrollToBottom()">Scroll to bottom</a>
			</span>

			<span>
				Filter level: &nbsp;
				<select id="filterLevelSelector" onchange="filterLevelChanged();">
					<option value="0">All</option>
					<option value="1">Emergency</option>
					<option value="2">Alert</option>
					<option value="3">Critical</option>
					<option value="4">Error</option>
					<option value="5">Warning</option>
					<option value="6">Notice</option>
					<option value="7">Info</option>
					<option value="8">Debug</option>
				</select>
			</span>
			&nbsp; &nbsp;
			<span>
				Find in syslog: &nbsp;
				<input type="text" id="log-find-text" autocomplete="off" title="Press Escape to clear search">
				<span style="visibility:hidden" id="log-occurence-span">Occurences: <b><span id="log-occurence-value"></span></b></span>
			</span>

		</div>

		<div class="tomato-grid" id="log-table-header"></div>
		<div class="tomato-grid" id="log-table" onscroll="onTableScroll();"></div>

		<div class="log-clear">
			<span>&raquo; <a href="logs/syslog.txt?_http_id=<% nv(http_id) %>">Download Log File</a></span><br>
			<span>&raquo; <a href="admin-log.asp">Logging Configuration</a></span><br><br>
		</div>

		<div class="log-viewlast">
			&raquo; <a href="javascript:viewLast(0)" id="entries0">View all</a><br>
			&raquo; <a href="javascript:viewLast(25)" id="entries25">View last 25 entries</a><br>
			&raquo; <a href="javascript:viewLast(50)" id="entries50">View last 50 entries</a><br>
			&raquo; <a href="javascript:viewLast(100)" id="entries100">View last 100 entries</a><br>
		</div>
	</div>
</div>

<!-- / / / -->

<div class="note-disabled" id="note-disabled" style="display:none"><b>Internal logging disabled.</b><br><br><a href="admin-log.asp">Enable &raquo;</a></div>

<!-- / / / -->

<div id="footer">
	<div class="log-controls">
		<span id="footer-msg2" style="display:none">Highlighted row copied to clipboard.</span>
		<img src="spin.gif" alt="" id="refresh-spinner">
		<script>genStdTimeList('refresh-time', 'Refresh Every', 0);</script>
		<input type="button" value="Refresh" onclick="ref.toggle()" id="refresh-button">
	</div>
</div>

</td></tr>
</table>
</form>
</body>
</html>
