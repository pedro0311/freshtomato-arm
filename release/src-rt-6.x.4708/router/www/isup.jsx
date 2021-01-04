isup = {};

/* OPENVPN-BEGIN */
isup.vpnserver1 = parseInt('<% psup("vpnserver1"); %>');
isup.vpnserver2 = parseInt('<% psup("vpnserver2"); %>');
/* OPENVPN-END */
/* PPTPD-BEGIN */
isup.pptpd = parseInt('<% psup("pptpd"); %>');
/* PPTPD-END */
