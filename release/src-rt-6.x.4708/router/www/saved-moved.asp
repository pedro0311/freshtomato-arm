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
<title>[<% ident(); %>] Restarting...</title>
<script type="text/javascript">
var n = 20;
function tick() {
    var e = document.getElementById("continue");
    e.value = n;
    if (n == 10) {
        e.style = "cursor:pointer";
        e.disabled = false;
    }
    if (n == 0) {
        e.value = "Continue";
    } else {
        --n;
        setTimeout(tick, 1000);
    }
}
function go() {
    window.location = window.location.protocol + '//<% nv("lan_ipaddr"); %>/';
}
</script>
<style type="text/css">
body {
    background:rgb(0,0,0) url(tomatousb_bg.png);
    font:14px Tahoma,Arial,sans-serif;
    color:rgb(255,255,255);
}
input {
    width:80px;
    height:24px;
}
.div {
    width:600px;
    height:70px;
    background-color:rgb(47,61,64);
    position:absolute;
    top:0;
    bottom:0;
    left:0;
    right:0;
    text-align:center;
    margin:auto;
    padding:10px 10px;
    border-radius:5px;
}
</style>
</head>
<body onload="tick()">
    <div class="div">
        <form action="">
            <div style="border-bottom:1px solid #aaa;margin:auto auto 5px;padding:0 0 5px;font-size:14px">
                The router's new IP address is <% nv("lan_ipaddr"); %>. You may need to release then renew your computer's DHCP lease before continuing.
            </div>
            <div id="but" style="display:inline-block">
                Please wait while the router restarts... &nbsp;
                <input type="button" value="" id="continue" onclick="go()" disabled="disabled">
            </div>
        </form>
    </div>
</body>
</html>