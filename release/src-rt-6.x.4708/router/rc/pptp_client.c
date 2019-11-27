/*
  PPTP CLIENT start/stop and configuration for Tomato
  by Jean-Yves Avenard (c) 2008-2011
*/

#include "rc.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 128
#define IF_SIZE 8

/* Line number as text string */
#define __LINE_T__ __LINE_T_(__LINE__)
#define __LINE_T_(x) __LINE_T(x)
#define __LINE_T(x) # x

#define vpnlog(level,x...) if (nvram_get_int("pptp_debug")>=level) syslog(level, __LINE_T__ ": " x)

void start_pptp_client(void)
{
	FILE *fd;
	int ok = 0;
	int i;
	char *p;
	char buffer[BUF_SIZE];
	char *argv[5];
	int argc = 0;

	vpnlog(LOG_DEBUG,"IN start_pptp_client...");

	struct hostent *he;
	struct in_addr **addr_list;

	char *srv_addr = nvram_safe_get("pptp_client_srvip");

	sprintf(buffer, "pptpclient");
	if (pidof(buffer) >= 0) {
		/* PPTP already running */
		vpnlog(LOG_DEBUG,"PPTP already running... stop");
		stop_pptp_client();
	}

	unlink("/etc/vpn/pptpc_ip-up");
	unlink("/etc/vpn/pptpc_ip-down");
	unlink("/etc/vpn/pptpc_options");
	unlink("/etc/vpn");
	unlink("/tmp/ppp");
	mkdir("/tmp/ppp",0700);
	mkdir("/etc/vpn",0700);
	ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-up");
	ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-down");
	/* Make sure symbolic link exists */
	sprintf(buffer, "/etc/vpn/pptpclient");
	unlink(buffer);
	ok |= symlink("/usr/sbin/pppd", buffer);
	if (ok) {
		stop_pptp_client();
		return;
	}
	/* Get IP from hostname */
	if (inet_addr(srv_addr) == INADDR_NONE) {
		if ((he = gethostbyname(srv_addr)) == NULL) {
			return;
		}
		addr_list = (struct in_addr **)he->h_addr_list;
		if (inet_ntoa(*addr_list[0]) == NULL) {
			return;
		}
		srv_addr = inet_ntoa(*addr_list[0]);
	}
	/* Generate ppp options */
	if ((fd = fopen("/etc/vpn/pptpc_options", "w")) != NULL) {
		ok = 1;
		fprintf(fd,
			"lock\n"
			"noauth\n"
			"refuse-eap\n"
			"lcp-echo-interval 10\n"
			"lcp-echo-failure 5\n"
			"maxfail 0\n"
			"persist\n"
			"plugin pptp.so\n"
			"pptp_server %s\n", srv_addr);
		i = nvram_get_int("pptp_client_peerdns");	/* 0: disable, 1 enable */
		if (i > 0)
			fprintf(fd,"usepeerdns\n");
		fprintf(fd,"idle 0\n"
			"ip-up-script /etc/vpn/pptpc_ip-up\n"
			"ip-down-script /etc/vpn/pptpc_ip-down\n"
			"ipparam kelokepptpd\n");
		/* MTU */
		if ((p = nvram_get("pptp_client_mtu")) == NULL)
			p = "1450";
		if (!nvram_get_int("pptp_client_mtuenable"))
			p = "1450";
		fprintf(fd,"mtu %s\n", p);
		/* MRU */
		if ((p = nvram_get("pptp_client_mru")) == NULL)
			p = "1450";
		if (!nvram_get_int("pptp_client_mruenable"))
			p = "1450";
		fprintf(fd,"mru %s\n", p);
		/* Login */
		if ((p = nvram_get("pptp_client_username")) == NULL)
			ok = 0;
		else
			fprintf(fd,"user \"%s\"\n", p);
		/* Password */
		if ((p = nvram_get("pptp_client_passwd")) == NULL)
			ok = 0;
		else
			fprintf(fd,"password \"%s\"\n", p);
		/* Encryption */
		strcpy(buffer,"");
		switch (nvram_get_int("pptp_client_crypt"))
		{
			case 1:
				fprintf(fd, "nomppe\n");
				break;
			case 2:
				fprintf(fd, "nomppe-40\n");
				fprintf(fd, "require-mppe-128\n");
				break;
			case 3:
				fprintf(fd, "require-mppe-40\n");
				fprintf(fd, "require-mppe-128\n");
				break;
			default:
				break;
		}
		if (!nvram_get_int("pptp_client_stateless"))
			fprintf(fd, "mppe-stateful\n");
		else
			fprintf(fd, "nomppe-stateful\n");
		fprintf(fd, "unit 4\n");	/* UNIT (ppp4) */
		fprintf(fd, "linkname pptpc\n");	/* link name for ID */
		fprintf(fd, "%s\n", nvram_safe_get("pptp_client_custom"));
		fclose(fd);
	}
	if (ok) {
		/* force route to PPTP server via selected wan */
		char *prefix = nvram_safe_get("pptp_client_usewan");
		if ((*prefix) && strcmp(prefix,"none")) {
			//eval("ip", "rule", "del", "lookup", (char *)get_wan_unit(prefix), "pref", "120");
			//eval("ip", "rule", "add", "to", srv_addr, "lookup", (char *)get_wan_unit(prefix), "pref", "120");
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
			vpnlog(LOG_DEBUG,"cmd=%s (force route to PPTP server via selected wan)", buffer);
			system(buffer);
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "ip rule add to %s lookup %d pref 120", srv_addr, get_wan_unit(prefix));
			vpnlog(LOG_DEBUG,"cmd=%s (force route to PPTP server via selected wan)", buffer);
			system(buffer);
		}

		sprintf(buffer, "/etc/vpn/pptpclient file /etc/vpn/pptpc_options");
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) ) {
			stop_pptp_client();
			return;
		}
		f_write("/etc/vpn/pptpclient_connecting", NULL, 0, 0, 0);
	} else {
		stop_pptp_client();
	}
	vpnlog(LOG_DEBUG,"OUT start_pptp_client");
}

void stop_pptp_client(void)
{
	int argc;
	char *argv[8];
	char buffer[BUF_SIZE];
	char cmd[256];
	char *prefix = nvram_safe_get("pptp_client_usewan");

	vpnlog(LOG_DEBUG,"IN stop_pptp_client...");
	//eval("/etc/vpn/pptpc_ip-down"); // why do it twice?
	killall_tk("pptpc_ip-up");
	killall_tk("pptpc_ip-down");
	killall_tk("pptpclient");

	/* remove forced route to PPTP server via selected wan */
	if ((*prefix) && strcmp(prefix,"none")) {
		//eval("ip", "rule", "del", "lookup", (char *)get_wan_unit(prefix), "pref", "120");
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
		vpnlog(LOG_DEBUG,"cmd=%s (PPTP server IP via selected WAN)", cmd);
		system(cmd);
	}

	sprintf(buffer, "rm -rf /etc/vpn/pptpclient /etc/vpn/pptpc_ip-down /etc/vpn/pptpc_ip-up /etc/vpn/pptpc_options /tmp/ppp/resolv.conf");
	for (argv[argc = 0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);
	rmdir("/etc/vpn");
	rmdir("/tmp/ppp");
	vpnlog(LOG_DEBUG,"OUT stop_pptp_client... /etc/vpn scripts removed!");
}

void append_pptp_route(void)
{
	if (nvram_get_int("pptp_client_dfltroute") == 1) {
		char cmd[256];
		char *pptp_client_ipaddr = nvram_safe_get("pptp_client_ipaddr");
		char *pptp_client_iface = nvram_safe_get("pptp_client_iface");
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route replace default scope global via %s dev %s", pptp_client_ipaddr, pptp_client_iface);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
		system(cmd);
	}
}

void clear_pptp_route(void)
{
/*
	struct hostent *he;
	struct in_addr **addr_list;
*/
	char cmd[256];
	char pmw[] = "wanXX";
	char *prefix = nvram_safe_get("pptp_client_usewan");

	vpnlog(LOG_DEBUG,"IN clear_pptp_route");

/*
	char *srv_addr = nvram_safe_get("pptp_client_srvip");

	if (inet_addr(srv_addr) == INADDR_NONE) {
		if ((he = gethostbyname(srv_addr)) == NULL)
		{
			return;
		}
		addr_list = (struct in_addr **)he->h_addr_list;
		if (inet_ntoa(*addr_list[0]) == NULL) {
			return;
		}
		srv_addr = inet_ntoa(*addr_list[0]);
	}

	// remove route to PPTP server
	eval("route", "del", srv_addr, "gw", wan_gateway(prefix), "dev", nvram_safe_get("pptp_client_iface"));
*/

	/* remove default route */
	if (nvram_get_int("pptp_client_dfltroute") == 1) {
		/* delete default route via PPTP */
		char *pptp_client_ipaddr = nvram_safe_get("pptp_client_ipaddr");
		char *pptp_client_iface = nvram_safe_get("pptp_client_iface");
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route del default via %s dev %s", pptp_client_ipaddr, pptp_client_iface);
		vpnlog(LOG_DEBUG,"cmd=%s", cmd);
		system(cmd);
		/* restore default route via binded WAN */
		char *wan_ipaddr, *wan_gw, *wan_iface;
		wanface_list_t wanfaces;
		vpnlog(LOG_DEBUG,"*** prefix: %s", prefix);
		if (check_wanup(prefix)) {
			wan_ipaddr = (char *)get_wanip(prefix);
			wan_gw = wan_gateway(prefix);
			memcpy(&wanfaces, get_wanfaces(prefix), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
		} else {	/* or via last primary wan */
			int num = nvram_get_int("wan_primary");
			get_wan_prefix(num, pmw);
			wan_ipaddr = (char *)get_wanip(pmw);
			wan_gw = wan_gateway(pmw);
			memcpy(&wanfaces, get_wanfaces(pmw), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
			prefix = pmw;
		}
		vpnlog(LOG_DEBUG,"*** [%s] ip:%s gw:%s iface:%s", prefix, wan_ipaddr, wan_iface, wan_gw);
		if (check_wanup(prefix)) {
			int proto = get_wanx_proto(prefix);
			memset(cmd, 0, 256);
			sprintf(cmd, "ip route add default via %s dev %s", (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_gw : wan_ipaddr,
				wan_iface);
			vpnlog(LOG_DEBUG,"cmd=%s", cmd);
			system(cmd);
		}
	}
	vpnlog(LOG_DEBUG,"OUT clear_pptp_route");
}

int write_pptpvpn_resolv(FILE* f)
{
	FILE *dnsf;
	int usepeer;
	int ch;

	if ((usepeer = nvram_get_int("pptp_client_peerdns")) <= 0) {
		vpnlog(LOG_DEBUG, "pptpclient: pptp peerdns disabled");
		return 0;
	}
	if (nvram_get_int("pptp_client_enable")) {	/* write DNS only for enabled client */
		dnsf = fopen("/tmp/ppp/resolv.conf", "r");
		if (dnsf == NULL) {
			vpnlog(LOG_DEBUG, "pptpclient: /tmp/ppp/resolv.conf can't be opened");
			return 0;
		}
		fputs("# pptp client dns:\n", f);
		while (!feof(dnsf)) {
			ch = fgetc(dnsf);
			fputc(ch == EOF ? '\n' : ch, f);
		}
		fclose(dnsf);
	}
	return (usepeer == 2) ? 1 : 0;
}

void pptp_rtbl_clear(void)
{
	int i;
	char cmd[256];
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref %d", PPTP_CLIENT_TABLE_ID, 115);
	for (i = 0; i <= 1000; i++) {	/* clean old rules */
		system(cmd);
	}
}

void pptp_rtbl_export(void)
{
	char *nvp, *b;
	char cmd[256];

	pptp_rtbl_clear();
	nvp = strdup(nvram_safe_get("pptp_client_rtbl"));
	while ((b = strsep(&nvp, "\n")) != NULL) {
		if (!strcmp(b,"")) continue;
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add to %s table %d pref %d", b, PPTP_CLIENT_TABLE_ID, 115);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
		system(cmd);
	}
}

void pptp_client_table_del(void)
{
	char cmd[256];

	/* ip route flush table PPTP (remove all PPTP routes) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route flush table %s", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
	system(cmd);

	/* ip rule del table PPTP pref 105 (from PPTP_IP) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %s pref 10%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
	system(cmd);

	/* ip rule del table PPTP pref 110 (to PPTP_DNS) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %s pref 110", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
	system(cmd); // del PPTP DNS1
	system(cmd); // del PPTP DNS2
/*
	// ip rule del from all to all lookup PPTP pref 120 (DEFAULT)
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del from all to all lookup %s pref 120", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
	system(cmd);
*/
	/* ip rule del fwmark 0x500/0xf00 table PPTP pref 125 (FWMARK) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %s pref 12%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
	system(cmd);

}

/* set pptp_client ip route table & ip rule table */
void pptp_client_table_add(void)
{
	vpnlog(LOG_DEBUG,"IN fun pptp_client_table_add");

	int mwan_num,i;
	char cmd[256];
	int wanid;
	char ip_cidr[32];
	char remote_cidr[32];
	char sPrefix[] = "wanXX";
	char tmp[100];
	int proto;

	char *pptp_client_ipaddr = nvram_safe_get("pptp_client_ipaddr");
	char *pptp_client_gateway = nvram_safe_get("pptp_client_gateway");
	char *pptp_client_iface = nvram_safe_get("pptp_client_iface");
	const dns_list_t *pptp_dns = get_dns("pptp_client");

	pptp_client_table_del();

	mwan_num = nvram_get_int("mwan_num");

	/*
	 * RULES
	 */

	/* ip rule add from PPTP_IP table PPTP pref 105 */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule add from %s table %s pref 10%d", pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (from PPTP_IP)", cmd);
	system(cmd);

	for (i = 0 ; i < pptp_dns->count; ++i) {
		/* ip rule add to PPTP_DNS table PPTP pref 110 */
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add to %s table %s pref 110", inet_ntoa(pptp_dns->dns[i].addr), PPTP_CLIENT_TABLE_NAME);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (to PPTP_DNS)", cmd);
		system(cmd);
	}
/*
	// ip rule add from all to all lookup PPTP pref 120
	if (nvram_get_int("pptp_client_dfltroute") == 1) {
		memset(cmd, 0, 256);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=ip rule add from all to all lookup %s pref 120 (from all to all: redirect Internet)", PPTP_CLIENT_TABLE_NAME);
		sprintf(cmd, "ip rule add from all to all lookup %s pref 120", PPTP_CLIENT_TABLE_NAME);
		system(cmd);
	}
	else {
		memset(cmd, 0, 256);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=ip rule del from all to all lookup %s pref 120 (from all to all: redirect Internet)", PPTP_CLIENT_TABLE_NAME);
		sprintf(cmd, "ip rule del from all to all lookup %s pref 120", PPTP_CLIENT_TABLE_NAME);
		system(cmd);
	}
*/
	/* ip rule add fwmark 0x500/0xf00 table PPTP pref 125 */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule add fwmark 0x%d00/0xf00 table %s pref 12%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (fwmark)", cmd);
	system(cmd);

	/*
	 * ROUTES
	 */

	/* all active WANX in PPTP table ? FIXME! check iface / ifname / gw for various WAN types */
	for (wanid = 1 ; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			proto = get_wanx_proto(sPrefix);
			memset(cmd, 0, 256);
			if (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) {
				/* wan ip/netmask, wan_iface, wan_ipaddr */
				get_cidr(nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)), nvram_safe_get(strcat_r(sPrefix, "_netmask",tmp)), ip_cidr);
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s",
					ip_cidr,	/* wan ip / netmask */
					nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)),	/* wan_iface */
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)),	/* wan_ipaddr */
					PPTP_CLIENT_TABLE_NAME);
			}
			else if ((proto == WP_PPTP || proto == WP_L2TP || proto == WP_PPPOE) && using_dhcpc(sPrefix)) {
				/* MAN: wan_gateway, wan_ifname, wan_ipaddr */
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strcat_r(sPrefix, "_gateway",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ifname",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)),
					PPTP_CLIENT_TABLE_NAME);
				vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (active MANX)", cmd);
				system(cmd);
				/* WAN: wan_gateway_get, wan_iface, wan_ppp_get_ip */
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strcat_r(sPrefix, "_gateway_get",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ppp_get_ip",tmp)),
					PPTP_CLIENT_TABLE_NAME);
			}
			else {
				/* wan gateway, wan_iface, wan_ipaddr */
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s",
					wan_gateway(sPrefix), 
					nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)),
					PPTP_CLIENT_TABLE_NAME);
			}
			vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (active WANX)", cmd);
			system(cmd);
		}
	}
	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (pptp gw)", cmd);
	system(cmd);
	for (wanid = 1 ; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			memset(cmd, 0, 256);
			sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, wanid);
			vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (pptp gw for %s)", cmd, sPrefix);
			system(cmd);
		}
	}
	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 table PPTP (pptp gw) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (pptp gw)", cmd);
	system(cmd);
	/* ip route add 192.168.1.0/24 dev br0 proto kernel scope link src 192.168.1.1 table PPTP (LAN) */
	get_cidr(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), ip_cidr);
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %s", ip_cidr, nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"), PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (LAN)", cmd);
	system(cmd);
	/* ip route add 127.0.0.0/8 dev lo scope link table PPTP (lo setup) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append 127.0.0.0/8 dev lo scope link table %s", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (lo setup)", cmd);
	system(cmd);
	/* ip route add default via 10.0.10.1 dev ppp3 table PPTP (default route) */
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append default via %s dev %s table %s", pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (default route)", cmd);
	system(cmd);
	/* PPTP network */
	if (!nvram_match("pptp_client_srvsub", "0.0.0.0") && !nvram_match("pptp_client_srvsubmsk", "0.0.0.0")) {
		/* add PPTP network to main table */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, "main");
		vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
		system(cmd);
		/* add PPTP network to all WANX tables */
		for (wanid = 1 ; wanid <= mwan_num; ++wanid) {
			get_wan_prefix(wanid, sPrefix);
			if (check_wanup(sPrefix)) {
				memset(cmd, 0, 256);
				sprintf(cmd, "ip route append %s via %s dev %s scope link table %d", remote_cidr, pptp_client_ipaddr, pptp_client_iface, wanid);
				vpnlog(LOG_DEBUG,"pptp_client, cmd=%s (%s)", cmd, sPrefix);
				system(cmd);
			}
		}
		/* add PPTP network to table PPTP */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
		vpnlog(LOG_DEBUG,"pptp_client, cmd=%s", cmd);
		system(cmd);
	}

	vpnlog(LOG_DEBUG,"OUT fun pptp_client_table_add");
}

int pptpc_ipup_main(int argc, char **argv)
{
	char *pptp_client_ifname;
	const char *p;
	char buf[256];
	struct sysinfo si;

	TRACE_PT("begin\n");

	vpnlog(LOG_DEBUG,"IN pptpc_ipup_main IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", getenv("IFNAME"), getenv("DEVICE"), getenv("LINKNAME"), getenv("IPREMOTE"), getenv("IPLOCAL"), getenv("DNS1"), getenv("DNS2"));

	sysinfo(&si);
	f_write("/etc/vpn/pptpclient_time", &si.uptime, sizeof(si.uptime), 0, 0);
	unlink("/etc/vpn/pptpclient_connecting");

	//killall("listen", SIGKILL);

	if (!wait_action_idle(10)) return -1;

	/* ipup receives six arguments:
	 * <interface name> <tty device> <speed> <local IP address> <remote IP address> <ipparam>
	 * ppp1 vlan1 0 71.135.98.32 151.164.184.87 0
	 */

	pptp_client_ifname = safe_getenv("IFNAME");
	if ((!pptp_client_ifname) || (!*pptp_client_ifname)) return -1;

	nvram_set("pptp_client_iface", pptp_client_ifname);	/* ppp# */
	nvram_set("pptp_client_pppd_pid", safe_getenv("PPPD_PID"));

	f_write_string("/etc/vpn/pptpclient_link", argv[1], 0, 0);

	if ((p = getenv("IPLOCAL"))) {
		_dprintf("IPLOCAL=%s\n", p);
		nvram_set("pptp_client_ipaddr", p);
		nvram_set("pptp_client_netmask", "255.255.255.255");
	}

	if ((p = getenv("IPREMOTE"))) {
		_dprintf("IPREMOTE=%s\n", p);
		nvram_set("pptp_client_gateway", p);
	}

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL) {
		strlcpy(buf, p, sizeof(buf));
	}
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0]) strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	nvram_set("pptp_client_get_dns", buf);
	TRACE_PT("DNS=%s\n", buf);

	pptp_client_table_add();
	pptp_rtbl_export();
	append_pptp_route();

	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	TRACE_PT("end\n");
	return 0;
}

int pptpc_ipdown_main(int argc, char **argv)
{
	TRACE_PT("begin\n");

	vpnlog(LOG_DEBUG,"IN pptpc_ipdown_main IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", getenv("IFNAME"), getenv("DEVICE"), getenv("LINKNAME"), getenv("IPREMOTE"), getenv("IPLOCAL"), getenv("DNS1"), getenv("DNS2"));

	if (!wait_action_idle(10)) return -1;

	pptp_client_table_del();
	pptp_rtbl_clear();
	clear_pptp_route();

	unlink("/etc/vpn/pptpclient_link");
	unlink("/etc/vpn/pptpclient_time");
	unlink("/etc/vpn/pptpclient_connecting");

	nvram_set("pptp_client_iface", "");
	nvram_set("pptp_client_pppd_pid", "");
	nvram_set("pptp_client_ipaddr", "0.0.0.0");
	nvram_set("pptp_client_netmask", "0.0.0.0");
	nvram_set("pptp_client_gateway", "0.0.0.0");
	nvram_set("pptp_client_get_dns", "");

	//killall("listen", SIGKILL);
	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	TRACE_PT("end\n");
	return 1;
}
