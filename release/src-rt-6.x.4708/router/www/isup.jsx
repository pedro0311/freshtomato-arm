//	<% etherstates(); %>

isup = {};

isup.time = '<% time(); %>';

isup.telnetd = parseInt('<% psup("telnetd"); %>');

isup.miniupnpd = parseInt('<% psup("miniupnpd"); %>');

/* it should be done in a different way, but for now it's ok */
isup.qos = <% nv("qos_enable"); %>;
isup.bwl = <% nv("bwl_enable"); %>;

/* OPENVPN-BEGIN */
var OVPN_CLIENT_COUNT = 3;
isup.vpnclient1 = parseInt('<% psup("vpnclient1"); %>');
isup.vpnclient2 = parseInt('<% psup("vpnclient2"); %>');
isup.vpnclient3 = parseInt('<% psup("vpnclient3"); %>');
var OVPN_SERVER_COUNT = 2;
isup.vpnserver1 = parseInt('<% psup("vpnserver1"); %>');
isup.vpnserver2 = parseInt('<% psup("vpnserver2"); %>');
/* OPENVPN-END */

/* PPTPD-BEGIN */
isup.pptpclient = parseInt('<% psup("pptpclient"); %>');
isup.pptpd = parseInt('<% psup("pptpd"); %>');
/* PPTPD-END */

/* NGINX-BEGIN */
isup.nginx = parseInt('<% psup("nginx"); %>');
/* NGINX-END */

/* SSH-BEGIN */
isup.dropbear = parseInt('<% psup("dropbear"); %>');
/* SSH-END */

/* MEDIA-SRV-BEGIN */
isup.minidlna = parseInt('<% psup("minidlna"); %>');
/* MEDIA-SRV-END */

/* TINC-BEGIN */
isup.tincd = parseInt ('<% psup("tincd"); %>');
/* TINC-END */
