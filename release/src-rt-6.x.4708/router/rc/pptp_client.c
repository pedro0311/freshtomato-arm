/*
  PPTP CLIENT start/stop and configuration for Tomato
  by Jean-Yves Avenard (c) 2008-2011
*/

#include "rc.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

//#define PPPD_DEBUG
#define BUF_SIZE 128

/* Line number as text string */
#define __LINE_T__ __LINE_T_(__LINE__)
#define __LINE_T_(x) __LINE_T(x)
#define __LINE_T(x) # x
#define vpnlog(level, x...) if (nvram_get_int("pptp_debug") >= level) syslog(level, __LINE_T__ ": " x)


void start_pptp_client(void)
{
	FILE *fd;
	int ok = 0;
	char *p;
	char buffer[BUF_SIZE];
	char *argv[5];
	int argc = 0;

	vpnlog(LOG_DEBUG, "IN start_pptp_client...");

	struct hostent *he;
	struct in_addr **addr_list;

	char *srv_addr = nvram_safe_get("pptp_client_srvip");

	if (pidof("pptpclient") >= 0) {
		/* PPTP already running */
		vpnlog(LOG_DEBUG, "PPTP already running... stop");
		stop_pptp_client();
	}

	unlink("/etc/vpn/pptpc_ip-up");
	unlink("/etc/vpn/pptpc_ip-down");
	unlink("/etc/vpn/pptpc_options");
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/vpn/pptpclient");
	unlink(buffer);

	/* Make sure vpn/ppp directory exists */
	mkdir("/etc/vpn", 0700);
	mkdir("/tmp/ppp", 0700);

	/* Make sure symbolic link exists */
	ok |= symlink("/usr/sbin/pppd", buffer);
	ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-up");
	ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-down");
	if (ok) {
		vpnlog(LOG_ERR, "Creating symlink failed...");
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
			"pptp_server %s\n"
			"idle 0\n"
			"ipparam kelokepptpd\n",
			srv_addr);

		if (nvram_get_int("pptp_client_peerdns"))	/* 0: disable, 1 enable */
			fprintf(fd, "usepeerdns\n");

		/* MTU */
		if ((p = nvram_get("pptp_client_mtu")) == NULL)
			p = "1450";
		if (!nvram_get_int("pptp_client_mtuenable"))
			p = "1450";
		fprintf(fd, "mtu %s\n", p);

		/* MRU */
		if ((p = nvram_get("pptp_client_mru")) == NULL)
			p = "1450";
		if (!nvram_get_int("pptp_client_mruenable"))
			p = "1450";
		fprintf(fd, "mru %s\n", p);

		/* Login */
		if ((p = nvram_get("pptp_client_username")) == NULL)
			ok = 0;
		else
			fprintf(fd, "user \"%s\"\n", p);

		/* Password */
		if ((p = nvram_get("pptp_client_passwd")) == NULL)
			ok = 0;
		else
			fprintf(fd, "password \"%s\"\n", p);

		/* Encryption */
		strcpy(buffer, "");
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

		fprintf(fd,
			"unit 4\n"		/* UNIT (ppp4) */
			"linkname pptpc\n"	/* link name for ID */
			"ip-up-script /etc/vpn/pptpc_ip-up\n"
			"ip-down-script /etc/vpn/pptpc_ip-down\n"
			"%s\n",
			nvram_safe_get("pptp_client_custom"));

		fclose(fd);
	}
	if (ok) {
		/* force route to PPTP server via selected wan */
		char *prefix = nvram_safe_get("pptp_client_usewan");
		if ((*prefix) && strcmp(prefix, "none")) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
			vpnlog(LOG_DEBUG, "cmd=%s (clean route to PPTP server via selected WAN)", buffer);
			system(buffer);
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "ip rule add to %s lookup %d pref 120", srv_addr, get_wan_unit(prefix));
			vpnlog(LOG_DEBUG, "cmd=%s (force route to PPTP server via selected WAN)", buffer);
			system(buffer);
		}

#ifdef PPPD_DEBUG
		sprintf(buffer, "/etc/vpn/pptpclient file /etc/vpn/pptpc_options debug");
#else
		sprintf(buffer, "/etc/vpn/pptpclient file /etc/vpn/pptpc_options");
#endif
		for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if (_eval(argv, NULL, 0, NULL)) {
			vpnlog(LOG_ERR, "Creating pptp tunnel failed...");
			stop_pptp_client();
			return;
		}
		f_write("/etc/vpn/pptpclient_connecting", NULL, 0, 0, 0);
	}
	else {
		vpnlog(LOG_ERR, "Found error in configuration - aborting...");
		stop_pptp_client();
	}

	vpnlog(LOG_DEBUG, "OUT start_pptp_client");
}

void stop_pptp_client(void)
{
	int argc;
	char *argv[8];
	char buffer[BUF_SIZE];
	char *prefix = nvram_safe_get("pptp_client_usewan");

	killall_tk("pptpc_ip-up");
	killall_tk("pptpc_ip-down");
	killall_tk("pptpclient");

	/* remove forced route to PPTP server via selected wan */
	if ((*prefix) && strcmp(prefix, "none")) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
		vpnlog(LOG_DEBUG,"stop_pptp_client, cmd=%s (clean route to PPTP server via selected WAN)", buffer);
		system(buffer);
	}

	vpnlog(LOG_DEBUG, "Removing generated files.");

	/* Delete all files for this client */
	unlink("/etc/vpn/pptpclient_connecting");
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "rm -rf /etc/vpn/pptpclient /etc/vpn/pptpc_ip-down /etc/vpn/pptpc_ip-up /etc/vpn/pptpc_options /tmp/ppp/resolv.conf");
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Attempt to remove directories. Will fail if not empty */
	rmdir("/etc/vpn");
	rmdir("/tmp/ppp");

	vpnlog(LOG_DEBUG, "Done removing generated files.");
}

void start_pptp_client_eas(void)
{
	if (nvram_get_int("pptp_client_eas"))
		start_pptp_client();
	else
		stop_pptp_client();
}

void stop_pptp_client_eas(void)
{
	if (nvram_get_int("pptp_client_eas"))
		stop_pptp_client();
}

void pptp_client_firewall(const char *table, const char *opt, _tf_ipt_write ipt_writer)
{
	char *pptpcface = nvram_safe_get("pptp_client_iface");
	char *srvsub = nvram_safe_get("pptp_client_srvsub");
	char *srvsubmsk = nvram_safe_get("pptp_client_srvsubmsk");
	int dflroute = nvram_get_int("pptp_client_dfltroute");

	if (pidof("pptpclient") < 0 || (!pptpcface) || (!*pptpcface) || (!strcmp(pptpcface, "none")))
		return;

	if ((!strcmp(table, "OUTPUT")) && dflroute) {
		ipt_writer("-A OUTPUT -d %s/%s -o %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
	}
	else if ((!strcmp(table, "FORWARD")) && dflroute) {
		ipt_writer("-A FORWARD -d %s/%s -o %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
		ipt_writer("-A FORWARD -s %s/%s -i %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
	}
	else if ((!strcmp(table, "INPUT")) && dflroute) {
		ipt_writer("-A INPUT -s %s/%s -i %s -j ACCEPT\n", srvsub, srvsubmsk, pptpcface);
	}
	else if ((!strcmp(table, "POSTROUTING")) && nvram_get_int("pptp_client_nat")) {
		/* PPTP Client NAT */
		if (nvram_get_int("ne_snat") != 1)
			ipt_writer("-A POSTROUTING %s -o %s -j MASQUERADE\n", opt, pptpcface);
		else
			ipt_writer("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", opt, pptpcface, nvram_safe_get("pptp_client_ipaddr"));
	}
}

void append_pptp_route(void)
{
	if (nvram_get_int("pptp_client_dfltroute") == 1) {
		char buffer[BUF_SIZE];
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip route replace default scope global via %s dev %s", nvram_safe_get("pptp_client_ipaddr"), nvram_safe_get("pptp_client_iface"));
		vpnlog(LOG_DEBUG, "append_pptp_route, cmd=%s", buffer);
		system(buffer);
	}
}

void clear_pptp_route(void)
{
	char buffer[BUF_SIZE];
	char pmw[] = "wanXX";
	char *prefix = nvram_safe_get("pptp_client_usewan");

	vpnlog(LOG_DEBUG, "IN clear_pptp_route");

	/* remove default route */
	if (nvram_get_int("pptp_client_dfltroute") == 1) {

		/* delete default route via PPTP */
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip route del default via %s dev %s", nvram_safe_get("pptp_client_ipaddr"), nvram_safe_get("pptp_client_iface"));
		vpnlog(LOG_DEBUG, "cmd=%s", buffer);
		system(buffer);

		char *wan_ipaddr, *wan_gw, *wan_iface;
		wanface_list_t wanfaces;

		vpnlog(LOG_DEBUG, "*** prefix: %s", prefix);

		/* restore default route via binded WAN */
		if (check_wanup(prefix)) {
			wan_ipaddr = (char *)get_wanip(prefix);
			wan_gw = wan_gateway(prefix);
			memcpy(&wanfaces, get_wanfaces(prefix), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
		}
		/* or via last primary wan */
		else {
			int num = nvram_get_int("wan_primary");
			get_wan_prefix(num, pmw);
			wan_ipaddr = (char *)get_wanip(pmw);
			wan_gw = wan_gateway(pmw);
			memcpy(&wanfaces, get_wanfaces(pmw), sizeof(wanfaces));
			wan_iface = wanfaces.iface[0].name;
			prefix = pmw;
		}

		vpnlog(LOG_DEBUG, "*** [%s] ip: %s gw: %s iface: %s", prefix, wan_ipaddr, wan_iface, wan_gw);

		if (check_wanup(prefix)) {
			int proto = get_wanx_proto(prefix);
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "ip route add default via %s dev %s", (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_gw : wan_ipaddr, wan_iface);
			vpnlog(LOG_DEBUG, "cmd=%s", buffer);
			system(buffer);
		}
	}

	vpnlog(LOG_DEBUG, "OUT clear_pptp_route");
}

int write_pptp_client_resolv(FILE* f)
{
	FILE *dnsf;
	int usepeer;
	int ch;

	if ((usepeer = nvram_get_int("pptp_client_peerdns")) <= 0) {
		vpnlog(LOG_DEBUG, "write_pptp_client_resolv: pptp peerdns disabled");
		return 0;
	}

	if (pidof("pptpclient") >= 0) {	/* write DNS only for active client */
		if (!(dnsf = fopen( "/tmp/ppp/resolv.conf", "r" ))) {
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

void pptp_client_table_del(void)
{
	char buffer[BUF_SIZE];

	/* ip route flush table PPTP (remove all PPTP routes) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route flush table %s", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client_table_del, cmd=%s", buffer);
	system(buffer);

	/* ip rule del table PPTP pref 105 (from PPTP_IP) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip rule del table %s pref 10%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG, "pptp_client_table_del, cmd=%s", buffer);
	system(buffer);

	/* ip rule del table PPTP pref 110 (to PPTP_DNS) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip rule del table %s pref 110", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client_table_del, cmd=%s", buffer);
	system(buffer);	/* del PPTP DNS1 */
	system(buffer);	/* del PPTP DNS2 */

	/* ip rule del fwmark 0x500/0xf00 table PPTP pref 125 (FWMARK) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip rule del table %s pref 12%d", PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG, "pptp_client_table_del, cmd=%s", buffer);
	system(buffer);

}

/* set pptp_client ip route table & ip rule table */
void pptp_client_table_add(void)
{
	vpnlog(LOG_DEBUG, "IN pptp_client_table_add");

	int mwan_num, i, wanid, proto;
	char buffer[BUF_SIZE];
	char ip_cidr[32];
	char remote_cidr[32];
	char sPrefix[] = "wanXX";
	char tmp[100];

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
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip rule add from %s table %s pref 10%d", pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (from PPTP_IP)", buffer);
	system(buffer);

	for (i = 0 ; i < pptp_dns->count; ++i) {
		/* ip rule add to PPTP_DNS table PPTP pref 110 */
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip rule add to %s table %s pref 110", inet_ntoa(pptp_dns->dns[i].addr), PPTP_CLIENT_TABLE_NAME);
		vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (to PPTP_DNS)", buffer);
		system(buffer);
	}

	/* ip rule add fwmark 0x500/0xf00 table PPTP pref 125 */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip rule add fwmark 0x%d00/0xf00 table %s pref 12%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_NAME, PPTP_CLIENT_TABLE_ID);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (fwmark)", buffer);
	system(buffer);

	/*
	 * ROUTES
	 */

	/* all active WANX in PPTP table ? FIXME! check iface / ifname / gw for various WAN types */
	for (wanid = 1; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			proto = get_wanx_proto(sPrefix);
			memset(buffer, 0, BUF_SIZE);
			if (proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) {
				/* wan ip/netmask, wan_iface, wan_ipaddr */
				get_cidr(nvram_safe_get(strcat_r(sPrefix, "_ipaddr", tmp)), nvram_safe_get(strcat_r(sPrefix, "_netmask", tmp)), ip_cidr);
				sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s",
					ip_cidr,	/* wan ip / netmask */
					nvram_safe_get(strcat_r(sPrefix, "_iface", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr", tmp)),
					PPTP_CLIENT_TABLE_NAME);
			}
			else if ((proto == WP_PPTP || proto == WP_L2TP || proto == WP_PPPOE) && using_dhcpc(sPrefix)) {
				/* MAN: wan_gateway, wan_ifname, wan_ipaddr */
				sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strcat_r(sPrefix, "_gateway", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ifname", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr", tmp)),
					PPTP_CLIENT_TABLE_NAME);
				vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (active MANX)", buffer);
				system(buffer);
				/* WAN: wan_gateway_get, wan_iface, wan_ppp_get_ip */
				sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s",
					nvram_safe_get(strcat_r(sPrefix, "_gateway_get", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_iface", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ppp_get_ip", tmp)),
					PPTP_CLIENT_TABLE_NAME);
			}
			else {
				/* wan gateway, wan_iface, wan_ipaddr */
				sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s",
					wan_gateway(sPrefix), 
					nvram_safe_get(strcat_r(sPrefix, "_iface", tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr", tmp)),
					PPTP_CLIENT_TABLE_NAME);
			}
			vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (active WANX)", buffer);
			system(buffer);
		}
	}

	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (pptp gw)", buffer);
	system(buffer);

	for (wanid = 1; wanid <= mwan_num; ++wanid) {
		get_wan_prefix(wanid, sPrefix);
		if (check_wanup(sPrefix)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %d", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, wanid);
			vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (pptp gw for %s)", buffer, sPrefix);
			system(buffer);
		}
	}

	/* ip route add 172.16.36.1 dev ppp4 proto kernel scope link src 172.16.36.13 table PPTP (pptp gw) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (pptp gw)", buffer);
	system(buffer);
	/* ip route add 192.168.1.0/24 dev br0 proto kernel scope link src 192.168.1.1 table PPTP (LAN) */
	get_cidr(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), ip_cidr);
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route append %s dev %s proto kernel scope link src %s table %s", ip_cidr, nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"), PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (LAN)", buffer);
	system(buffer);
	/* ip route add 127.0.0.0/8 dev lo scope link table PPTP (lo setup) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route append 127.0.0.0/8 dev lo scope link table %s", PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (lo setup)", buffer);
	system(buffer);
	/* ip route add default via 10.0.10.1 dev ppp3 table PPTP (default route) */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ip route append default via %s dev %s table %s", pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
	vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (default route)", buffer);
	system(buffer);

	/* PPTP network */
	if (!nvram_match("pptp_client_srvsub", "0.0.0.0") && !nvram_match("pptp_client_srvsubmsk", "0.0.0.0")) {
		/* add PPTP network to main table */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, "main");
		vpnlog(LOG_DEBUG, "pptp_client, cmd=%s", buffer);
		system(buffer);
		/* add PPTP network to all WANX tables */
		for (wanid = 1; wanid <= mwan_num; ++wanid) {
			get_wan_prefix(wanid, sPrefix);
			if (check_wanup(sPrefix)) {
				memset(buffer, 0, BUF_SIZE);
				sprintf(buffer, "ip route append %s via %s dev %s scope link table %d", remote_cidr, pptp_client_ipaddr, pptp_client_iface, wanid);
				vpnlog(LOG_DEBUG, "pptp_client, cmd=%s (%s)", buffer, sPrefix);
				system(buffer);
			}
		}
		/* add PPTP network to table PPTP */
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_NAME);
		vpnlog(LOG_DEBUG, "pptp_client, cmd=%s", buffer);
		system(buffer);
	}

	vpnlog(LOG_DEBUG, "OUT pptp_client_table_add");
}

int pptpc_ipup_main(int argc, char **argv)
{
	char *pptp_client_ifname;
	const char *p;
	struct sysinfo si;

	/* ipup receives six arguments:
	 * <interface name> <tty device> <speed> <local IP address> <remote IP address> <ipparam>
	 * ppp1 vlan1 0 71.135.98.32 151.164.184.87 0
	 */

	vpnlog(LOG_DEBUG, "IN pptpc_ipup_main IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", getenv("IFNAME"), getenv("DEVICE"), getenv("LINKNAME"), getenv("IPREMOTE"), getenv("IPLOCAL"), getenv("DNS1"), getenv("DNS2"));

	sysinfo(&si);
	f_write("/etc/vpn/pptpclient_time", &si.uptime, sizeof(si.uptime), 0, 0);
	unlink("/etc/vpn/pptpclient_connecting");

	pptp_client_ifname = safe_getenv("IFNAME");
	if ((!pptp_client_ifname) || (!*pptp_client_ifname) || (!wait_action_idle(10)))
		return -1;

	nvram_set("pptp_client_iface", pptp_client_ifname);	/* ppp# */

	f_write_string("/etc/vpn/pptpclient_link", argv[1], 0, 0);

	if ((p = getenv("IPLOCAL"))) {
		nvram_set("pptp_client_ipaddr", p);
		nvram_set("pptp_client_netmask", "255.255.255.255");
	}

	if ((p = getenv("IPREMOTE"))) {
		nvram_set("pptp_client_gateway", p);
	}

	pptp_client_table_add();
	append_pptp_route();

	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	vpnlog(LOG_DEBUG, "OUT pptpc_ipup_main");

	return 0;
}

int pptpc_ipdown_main(int argc, char **argv)
{
	vpnlog(LOG_DEBUG, "IN pptpc_ipdown_main IFNAME=%s DEVICE=%s LINKNAME=%s IPREMOTE=%s IPLOCAL=%s DNS1=%s DNS2=%s", getenv("IFNAME"), getenv("DEVICE"), getenv("LINKNAME"), getenv("IPREMOTE"), getenv("IPLOCAL"), getenv("DNS1"), getenv("DNS2"));

	if (!wait_action_idle(10))
		return -1;

	pptp_client_table_del();
	clear_pptp_route();

	unlink("/etc/vpn/pptpclient_link");
	unlink("/etc/vpn/pptpclient_time");
	unlink("/etc/vpn/pptpclient_connecting");

	nvram_set("pptp_client_iface", "");
	nvram_set("pptp_client_ipaddr", "0.0.0.0");
	nvram_set("pptp_client_netmask", "0.0.0.0");
	nvram_set("pptp_client_gateway", "0.0.0.0");

	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	vpnlog(LOG_DEBUG, "OUT pptpc_ipdown_main");

	return 1;
}
