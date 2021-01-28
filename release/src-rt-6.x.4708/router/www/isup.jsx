isup = {};

/* OPENVPN-BEGIN */
isup.vpnserver1 = parseInt('<% psup("vpnserver1"); %>');
isup.vpnserver2 = parseInt('<% psup("vpnserver2"); %>');
/* OPENVPN-END */
/* PPTPD-BEGIN */
isup.pptpd = parseInt('<% psup("pptpd"); %>');
/* PPTPD-END */
/* NGINX-BEGIN */
isup.nginx = parseInt('<% psup("nginx"); %>');
/* NGINX-END */

/* it should be done in a different way, but for now it's ok */
isup.qos = <% nv("qos_enable"); %>;
isup.bwl = <% nv("bwl_enable"); %>;
