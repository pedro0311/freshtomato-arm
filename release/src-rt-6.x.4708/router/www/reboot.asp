<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf-8">
<meta name="robots" content="noindex,nofollow">
<title>[<% ident(); %>] Rebooting...</title>
<script type="text/javascript">
var n = 130 + parseInt('0<% nv("wait_time"); %>');
function tick() {
    var e = document.getElementById("continue");
    e.value = n--;
    if (n < 0) {
        e.value = "Continue";
        return;
    }
    if (n == 19) {
        e.style = "cursor:pointer";
        e.disabled = false;
    }
    setTimeout(tick, 1000);
}

function go() {
    window.location.replace("/");
}

function init() {
    resmsg = '';
    //<% resmsg(); %>
    if (resmsg.length) {
        e = document.getElementById("msg");
        e.innerHTML = resmsg;
        e.style.display = "block";
    } else {
        e = document.getElementById("but");
        e.style = "margin:15px 0 0";
    }
    tick()
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
    height:55px;
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
<body onload="init()">
    <div class="div">
        <form action="">
            <div style="display:none;border-bottom:1px solid #aaa;margin:auto auto 5px;padding:0 0 5px;font-weight:bold" id="msg"></div>
            <div id="but" style="display:inline-block">
                Please wait while the router reboots... &nbsp;
                <input type="button" value="" id="continue" onclick="go()" disabled="disabled">
            </div>
        </form>
    </div>
</body>
</html>