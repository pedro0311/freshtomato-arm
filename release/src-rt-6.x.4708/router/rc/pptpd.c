/*
 * pptpd.c
 *
 * Copyright (C) 2007 Sebastian Gottschall <gottschall@dd-wrt.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id:
 */


#include "rc.h"

#include <stdlib.h>
#include <bcmnvram.h>
#include <shutils.h>
#include <utils.h>
#include <errno.h>
#include <sys/stat.h>

#define PPTPD_CONFFILE		"/etc/vpn/pptpd.conf"
#define PPTPD_OPTIONS		"/etc/vpn/pptpd_options"
#define PPTPD_UP_SCRIPT		"/etc/vpn/pptpd_ip-up"
#define PPTPD_DOWN_SCRIPT	"/etc/vpn/pptpd_ip-down"
#define PPTPD_SECRETS		"/etc/vpn/chap-secrets"
#define PPTPD_CONNECTED		"/etc/vpn/pptpd_connected"
#define PPTPD_SHUTDOWN		"/etc/vpn/pptpd_shutdown"


char *ip2bcast(char *ip, char *netmask, char *buf)
{
	struct in_addr addr;

	addr.s_addr = inet_addr(ip) | ~inet_addr(netmask);
	if (buf)
		sprintf(buf, "%s", inet_ntoa(addr));

	return buf;
}

void write_chap_secret(char *file)
{
	FILE *fp;
	char *nv, *nvp, *b;
	char *username, *passwd;

	if ((fp = fopen(file, "w")) == NULL) {
		perror(file);
		return;
	}

	nv = nvp = strdup(nvram_safe_get("pptpd_users"));

	if (nv) {
		while ((b = strsep(&nvp, ">")) != NULL) {
			if ((vstrsep(b, "<", &username, &passwd) < 2))
				continue;

			if (*username =='\0' || *passwd == '\0')
				continue;

			fprintf(fp, "%s * %s *\n", username, passwd);
		}
		free(nv);
	}
	fclose(fp);
}

void start_pptpd(void)
{
	FILE *fp;
	int count = 0, ret = 0, nowins = 0, pptpd_opt;
	char bcast[32];
#ifdef TCONFIG_BCMARM
	int ctf_disable = nvram_get_int("ctf_disable");
#endif /* TCONFIG_BCMARM */

	if (!nvram_match("pptpd_enable", "1"))
		return;

	/* Make sure vpn directory exists */
	mkdir("/etc/vpn", 0700);

	/* Create unique options file */
	if ((fp = fopen(PPTPD_OPTIONS, "w")) == NULL) {
		perror(PPTPD_OPTIONS);
		return;
	}

	fprintf(fp, "logfile /var/log/pptpd-pppd.log\n"
	            "debug\n");

#if 0
	if (nvram_match("pptpd_radius", "1") && nvram_invmatch("pptpd_radserver", "") && nvram_invmatch("pptpd_radpass", ""))
		fprintf(fp, "plugin radius.so\n"
		            "plugin radattr.so\n"
		            "radius-config-file /etc/vpn/radius/radiusclient.conf\n");
#endif

	fprintf(fp, "lock\n"
	            "name *\n"
	            "proxyarp\n"
	            //"ipcp-accept-local\n"
	            //"ipcp-accept-remote\n"
	            "lcp-echo-failure 10\n"
	            "lcp-echo-interval 5\n"
	            "lcp-echo-adaptive\n"
	            "auth\n"
	            "nobsdcomp\n"
	            "refuse-pap\n"
	            "refuse-chap\n"
	            "nomppe-stateful\n");

	pptpd_opt = nvram_get_int("pptpd_chap");
	fprintf(fp, "%s-mschap\n", (pptpd_opt == 0 || pptpd_opt & 1) ? "require" : "refuse");
	fprintf(fp, "%s-mschap-v2\n", (pptpd_opt == 0 || pptpd_opt & 2) ? "require" : "refuse");

	if (nvram_match("pptpd_forcemppe", "0"))
		fprintf(fp, "nomppe nomppc\n");
	else
		fprintf(fp, "require-mppe-128\n");

	fprintf(fp, "ms-ignore-domain\n"
	            "chap-secrets "PPTPD_SECRETS"\n"
	            "ip-up-script "PPTPD_UP_SCRIPT"\n"
	            "ip-down-script "PPTPD_DOWN_SCRIPT"\n"
	            "mtu %d\n"
	            "mru %d\n",
	            nvram_get_int("pptpd_mtu") ? : 1400,
	            nvram_get_int("pptpd_mru") ? : 1400);

	/* DNS Server */
	if (nvram_invmatch("pptpd_dns1", ""))
		count += fprintf(fp, "ms-dns %s\n", nvram_safe_get("pptpd_dns1")) > 0 ? 1 : 0;
	if (nvram_invmatch("pptpd_dns2", ""))
		count += fprintf(fp, "ms-dns %s\n", nvram_safe_get("pptpd_dns2")) > 0 ? 1 : 0;
	if (count == 0 && nvram_invmatch("lan_ipaddr", ""))
		fprintf(fp, "ms-dns %s\n", nvram_safe_get("lan_ipaddr"));

	/* WINS Server */
	if (nvram_match("wan_wins", "0.0.0.0") || (strlen(nvram_safe_get("wan_wins")) == 0)) {
		nvram_set("wan_wins", "");
		nowins = 1;
	}

	if (!nowins)
		fprintf(fp, "ms-wins %s\n", nvram_safe_get("wan_wins"));

	if (strlen(nvram_safe_get("pptpd_wins1")))
		fprintf(fp, "ms-wins %s\n", nvram_safe_get("pptpd_wins1"));

	if (strlen(nvram_safe_get("pptpd_wins2")))
		fprintf(fp, "ms-wins %s\n", nvram_safe_get("pptpd_wins2"));

	fprintf(fp, "minunit 10\n"		/* force ppp interface starting from 10 */
	            "%s\n\n", nvram_safe_get("pptpd_custom"));

	fclose(fp);

	/* Following is all crude and need to be revisited once testing confirms that it does work
	 * Should be enough for testing..
	 */
#if 0
	if (nvram_get_int("pptpd_radius") && nvram_invmatch("pptpd_radserver", "") && nvram_invmatch("pptpd_radpass", "")) {
		mkdir("/etc/vpn/radius", 0700);

		fp = fopen("/etc/vpn/radius/radiusclient.conf", "w");
		fprintf(fp, "auth_order radius\n"
		            "login_tries 4\n"
		            "login_timeout 60\n"
		            "radius_timeout 10\n"
		            "nologin /etc/nologin\n"
		            "servers /etc/vpn/radius/servers\n"
		            "dictionary /etc/dictionary\n"
		            "seqfile /var/run/radius.seq\n"
		            "mapfile /etc/port-id-map\n"
		            "radius_retries 3\n"
		            "authserver %s:%s\n",
		            nvram_safe_get("pptpd_radserver"),
		            nvram_get("pptpd_radport") ? nvram_get("pptpd_radport") : "radius");

		if ((nvram_get("pptpd_radserver") != NULL) && (nvram_get("pptpd_acctport") != NULL))
			fprintf(fp, "acctserver %s:%s\n",
			            nvram_safe_get("pptpd_radserver"),
			            nvram_get("pptpd_acctport") ? nvram_get("pptpd_acctport") : "radacct");

		fclose(fp);

		fp = fopen("/etc/vpn/radius/servers", "w");
		fprintf(fp, "%s\t%s\n",
		            nvram_safe_get("pptpd_radserver"),
		            nvram_safe_get("pptpd_radpass"));

		fclose(fp);
#endif

	/* Create pptpd.conf options file for pptpd daemon */
	fp = fopen(PPTPD_CONFFILE, "w");
	fprintf(fp, "localip %s\n"
	            "remoteip %s\n"
	            "bcrelay %s\n",
	            nvram_safe_get("lan_ipaddr"),
	            nvram_safe_get("pptpd_remoteip"),
	            nvram_safe_get("pptpd_broadcast"));

	fclose(fp);

	memset(bcast, 0, 32);
	ip2bcast(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), bcast);

	/* Create ip-up and ip-down scripts that are unique to pptpd to avoid interference with pppoe and pptpc */
	fp = fopen(PPTPD_UP_SCRIPT, "w");
	fprintf(fp, "#!/bin/sh\n"
	            "echo \"$PPPD_PID $1 $5 $6 $PEERNAME $(date +%%s)\" >> "PPTPD_CONNECTED"\n"
	            "iptables -I INPUT -i $1 -j ACCEPT\n"
	            "iptables -I FORWARD -i $1 -j ACCEPT\n"
	            "iptables -I FORWARD -o $1 -j ACCEPT\n"
	            "iptables -I FORWARD -i $1 -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n"
	            "iptables -t nat -I PREROUTING -i $1 -p udp -m udp --sport 9 -j DNAT --to-destination %s\n"	/* rule for wake on lan over pptp tunnel */
	            "%s\n",
	            bcast,
	            nvram_safe_get("pptpd_ipup_script"));
#ifdef TCONFIG_BCMARM
	if (!ctf_disable) /* bypass CTF if enabled */
		fprintf(fp, "iptables -t mangle -A FORWARD -i $1 -m state --state NEW -j MARK --set-mark 0x01/0x7\n");
#endif /* TCONFIG_BCMARM */

	fclose(fp);

	fp = fopen(PPTPD_DOWN_SCRIPT, "w");
	fprintf(fp, "#!/bin/sh\n" "grep -v $1 "PPTPD_CONNECTED" > "PPTPD_CONNECTED".new\n"
	            "mv "PPTPD_CONNECTED".new "PPTPD_CONNECTED"\n"
	            "iptables -D FORWARD -i $1 -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n"
	            "iptables -D INPUT -i $1 -j ACCEPT\n"
	            "iptables -D FORWARD -i $1 -j ACCEPT\n"
	            "iptables -D FORWARD -o $1 -j ACCEPT\n"
	            "iptables -t nat -D PREROUTING -i $1 -p udp -m udp --sport 9 -j DNAT --to-destination %s\n"	/* rule for wake on lan over pptp tunnel */
	            "%s\n",
	            bcast,
	            nvram_safe_get("pptpd_ipdown_script"));
#ifdef TCONFIG_BCMARM
	if (!ctf_disable) /* bypass CTF if enabled */
		fprintf(fp, "iptables -t mangle -D FORWARD -i $1 -m state --state NEW -j MARK --set-mark 0x01/0x7\n");
#endif /* TCONFIG_BCMARM */

	fclose(fp);

	chmod(PPTPD_UP_SCRIPT, 0744);
	chmod(PPTPD_DOWN_SCRIPT, 0744);

	/* Extract chap-secrets from nvram */
	write_chap_secret(PPTPD_SECRETS);

	chmod(PPTPD_SECRETS, 0600);

	/* Execute pptpd daemon */
	ret = eval("pptpd", "-c", PPTPD_CONFFILE, "-o", PPTPD_OPTIONS, "-C", "50");
}

void stop_pptpd(void)
{
	FILE *fp;
	int ppppid;
	char line[128];

	eval("cp", PPTPD_CONNECTED, PPTPD_SHUTDOWN);

	if ((fp = fopen(PPTPD_SHUTDOWN, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (sscanf(line, "%d %*s %*s %*s %*s %*d", &ppppid) != 1)
				continue;

			int n = 10;
			while ((kill(ppppid, SIGTERM) == 0) && (n > 1)) {
				sleep(1);
				n--;
			}
		}
		fclose(fp);
	}

	killall_tk_period_wait("pptpd", 50);
	killall_tk_period_wait("bcrelay", 50);

	/* Delete all files for this server */
	unlink(PPTPD_SHUTDOWN);

	eval("rm", "-rf", PPTPD_CONFFILE, PPTPD_OPTIONS, PPTPD_DOWN_SCRIPT, PPTPD_UP_SCRIPT, PPTPD_SECRETS);

	/* Attempt to remove directory. Will fail if not empty */
	rmdir("/etc/vpn");
}

void write_pptpd_dnsmasq_config(FILE* f) {
	if (nvram_match("pptpd_enable", "1"))
		fprintf(f, "interface=ppp1*\n"			/* Listen on the ppp1* interfaces (wildcard *); we start with 10 and up ...  see minunit 10 */
		           "no-dhcp-interface=ppp1*\n"		/* Do not provide DHCP, but do provide DNS service */
		           "interface=vlan*\n"			/* Listen on the vlan* interfaces (wildcard *) */
		           "no-dhcp-interface=vlan*\n"		/* Do not provide DHCP, but do provide DNS service */
		           "interface=eth*\n"			/* Listen on the eth* interfaces (wildcard *) */
		           "no-dhcp-interface=eth*\n");		/* Do not provide DHCP, but do provide DNS service */
}
