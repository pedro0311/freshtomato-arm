/*

	Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com

	No part of this file may be used without permission.

*/

#include "rc.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <time.h>

#define CLIENT_IF_START	10
#define SERVER_IF_START	20

#define BUF_SIZE	256
#define IF_SIZE		8

/* OpenVPN clients/servers count */
#define OVPN_SERVER_MAX	2

#if defined(TCONFIG_BCMARM)
#define OVPN_CLIENT_MAX	3
#else
#define OVPN_CLIENT_MAX	2
#endif

/* OpenVPN routing policy modes (rgw) */
enum {
	OVPN_RGW_NONE = 0,
	OVPN_RGW_ALL,
	OVPN_RGW_POLICY
};


static char *getNVRAMVar(const char *text, const int num)
{
	char buffer[32];
	memset(buffer, 0, 32);
	sprintf(buffer, text, num);

	return nvram_safe_get(buffer);
}

static int ovpn_waitfor(const char *name)
{
	int pid, n = 5;

	killall_tk_period_wait(name, 10); /* wait time in seconds */
	while ((pid = pidof(name)) >= 0 && (n-- > 0)) {
		/* Reap the zombie if it has terminated */
		waitpid(pid, NULL, WNOHANG);
		sleep(1);
	}

	return (pid >= 0);
}

void start_ovpn_client(int clientNum)
{
	FILE *fp;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[32];
	char *argv[6];
	int argc = 0;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	enum { TAP, TUN } ifType = TUN;
	enum { BRIDGE, NAT, NONE } routeMode = NONE;
	int nvi, ip[4], nm[4];
	long int nvl;
	int pid;
	int userauth, useronly;
	int i;
	int taskset_ret = 0;
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
#endif

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnclient%d", clientNum);
	if (getpid() != 1) {
		start_service(buffer);
		return;
	}

	if ((pid = pidof(buffer)) >= 0)
		return;

	/* Determine interface */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_if", clientNum);
	if (nvram_contains_word(buffer, "tap"))
		ifType = TAP;
	else if (nvram_contains_word(buffer, "tun"))
		ifType = TUN;
	else
		return;

	/* Build interface name */
	snprintf(iface, IF_SIZE, "%s%d", nvram_safe_get(buffer), (clientNum + CLIENT_IF_START));

	/* Determine encryption mode */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_crypt", clientNum);
	if (nvram_contains_word(buffer, "tls"))
		cryptMode = TLS;
	else if (nvram_contains_word(buffer, "secret"))
		cryptMode = SECRET;
	else if (nvram_contains_word(buffer, "custom"))
		cryptMode = CUSTOM;
	else
		return;

	/* Determine if we should bridge the tunnel */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_bridge", clientNum);
	if (ifType == TAP && nvram_get_int(buffer) == 1)
		routeMode = BRIDGE;

	/* Determine if we should NAT the tunnel */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_nat", clientNum);
	if (((ifType == TUN) || (routeMode != BRIDGE)) && nvram_get_int(buffer) == 1)
		routeMode = NAT;

	/* Make sure openvpn directory exists */
	mkdir("/etc/openvpn", 0700);
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/client%d", clientNum);
	mkdir(buffer, 0700);

	/* Make sure symbolic link exists */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/vpnclient%d", clientNum);
	unlink(buffer);
	if (symlink("/usr/sbin/openvpn", buffer)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Creating symlink failed...");
#endif
		stop_ovpn_client(clientNum);
		return;
	}

	/* Make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	/* Create tap/tun interface */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --mktun --dev %s", iface);
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Creating tunnel interface failed...");
#endif
		stop_ovpn_client(clientNum);
		return;
	}

	/* Bring interface up (TAP only) */
	if (ifType == TAP) {
		if (routeMode == BRIDGE) {
			memset(buffer, 0, BUF_SIZE);
			snprintf(buffer, BUF_SIZE, "brctl addif %s %s", getNVRAMVar("vpn_client%d_br", clientNum), iface);
			for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));

			if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
				syslog(LOG_WARNING, "Adding tunnel interface to bridge failed...");
#endif
				stop_ovpn_client(clientNum);
				return;
			}
		}

		snprintf(buffer, BUF_SIZE, "ifconfig %s promisc up", iface);
		for (argv[argc=0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));

		if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
			syslog(LOG_WARNING, "Bringing interface up failed...");
#endif
			stop_ovpn_client(clientNum);
			return;
		}
	}

	userauth = atoi(getNVRAMVar("vpn_server%d_userpass", clientNum));
	useronly = userauth && atoi(getNVRAMVar("vpn_server%d_nocert", clientNum));

	/* Build and write config file */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/client%d/config.ovpn", clientNum);
	fp = fopen(buffer, "w");
	chmod(buffer, (S_IRUSR | S_IWUSR));

	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-client%d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "persist-key\n"
	            "persist-tun\n",
	            clientNum,
	            iface);

	if (cryptMode == TLS)
		fprintf(fp, "client\n");

	fprintf(fp, "proto %s\n", getNVRAMVar("vpn_client%d_proto", clientNum)); /* full dual-stack functionality starting with OpenVPN 2.4.0 */
	fprintf(fp, "remote %s ", getNVRAMVar("vpn_client%d_addr", clientNum));
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_port", clientNum);
	fprintf(fp, "%d\n", nvram_get_int(buffer));

	if (cryptMode == SECRET) {
		fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_client%d_local", clientNum));

		if (ifType == TUN)
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_remote", clientNum));
		else if (ifType == TAP)
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_nm", clientNum));
	}

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_retry", clientNum);
	if ((nvi = nvram_get_int(buffer)) >= 0)
		fprintf(fp, "resolv-retry %d\n", nvi);
	else
		fprintf(fp, "resolv-retry infinite\n");

	if ((nvl = atol(getNVRAMVar("vpn_client%d_reneg", clientNum))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_nobind", clientNum);
	if (nvram_get_int(buffer) > 0)
		fprintf(fp, "nobind\n");

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_client%d_comp", clientNum), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		if ((!strcmp(buffer, "lz4")) || (!strcmp(buffer, "lz4-v2")))
			fprintf(fp, "compress %s\n", buffer);
		else
#endif
		     if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
#ifndef TCONFIG_OPTIMIZE_SIZE
		else if ((!strcmp(buffer, "stub")) || (!strcmp(buffer, "stub-v2")))
			fprintf(fp, "compress %s\n", buffer);
#endif
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n");	/* Disable, but can be overriden */
	}

	/* Cipher */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_client%d_ncp_ciphers", clientNum), sizeof(buffer));
	if (cryptMode == TLS) {
		if (buffer[0] != '\0')
#ifndef TCONFIG_OPTIMIZE_SIZE
			fprintf(fp, "data-ciphers %s\n", buffer);
#else
			fprintf(fp, "ncp-ciphers %s\n", buffer);
#endif
	}
#ifndef TCONFIG_OPTIMIZE_SIZE
	else {	/* SECRET/CUSTOM */
#endif
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_cipher", clientNum);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
#ifndef TCONFIG_OPTIMIZE_SIZE
	}
#endif

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_digest", clientNum);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Routing */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_rgw", clientNum);
	nvi = nvram_get_int(buffer);

	if (nvi == OVPN_RGW_ALL) {
		if (ifType == TAP && getNVRAMVar("vpn_client%d_gw", clientNum)[0] != '\0')
			fprintf(fp, "route-gateway %s\n", getNVRAMVar("vpn_client%d_gw", clientNum));
		fprintf(fp, "redirect-gateway def1\n");
	}
	else if (nvi >= OVPN_RGW_POLICY)
		fprintf(fp, "route-noexec\n");

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/client%d/updown-client.sh", clientNum);
	symlink("/usr/sbin/updown-client.sh", buffer);

	/* Selective routing */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/client%d/vpnrouting.sh", clientNum);
	symlink("/usr/sbin/vpnrouting.sh", buffer);
	fprintf(fp, "script-security 2\n"
	            "up updown-client.sh\n"
	            "down updown-client.sh\n"
	            "route-delay 2\n"
	            "route-up vpnrouting.sh\n"
	            "route-pre-down vpnrouting.sh\n");

	if (cryptMode == TLS) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_hmac", clientNum);
		nvi = nvram_get_int(buffer);
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_static", clientNum);

		if (!nvram_is_empty(buffer) && nvi >= 0) {
			if (nvi == 3)
				fprintf(fp, "tls-crypt static.key");
#ifndef TCONFIG_OPTIMIZE_SIZE
			else if (nvi == 4)
				fprintf(fp, "tls-crypt-v2 static.key");
#endif
			else
				fprintf(fp, "tls-auth static.key");

			if (nvi < 2)
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_ca", clientNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_client%d_crt", clientNum);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "cert client.crt\n");

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_client%d_key", clientNum);
			if (!nvram_is_empty(buffer))
				fprintf(fp, "key client.key\n");
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_tlsremote", clientNum);
		if (nvram_get_int(buffer))
			fprintf(fp, "remote-cert-tls server\n");

		if ((nvi = atoi(getNVRAMVar("vpn_server%d_tlsvername", clientNum))) > 0) {
			fprintf(fp, "verify-x509-name \"%s\" ", getNVRAMVar("vpn_client%d_cn", clientNum));
			if (nvi == 2)
				fprintf(fp, "name-prefix\n");
			else if (nvi == 3)
				fprintf(fp, "subject\n");
			else
				fprintf(fp, "name\n");
		}

		if (userauth)
			fprintf(fp, "auth-user-pass up\n");
	}
	else if (cryptMode == SECRET) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_static", clientNum);

		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	memset(buffer, 0, BUF_SIZE);
	fprintf(fp, "keepalive 15 60\n"
	            "verb 3\n"
	            "status-version 2\n"
	            "status status 10\n\n" /* Update status file every 10 sec */
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpn_client%d_custom", clientNum));

	fclose(fp);

	/* Write certification and key files */
	if (cryptMode == TLS) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_ca", clientNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/client%d/ca.crt", clientNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_client%d_ca", clientNum));
			fclose(fp);
		}

		if (!useronly) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_client%d_key", clientNum);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				sprintf(buffer, "/etc/openvpn/client%d/client.key", clientNum);
				fp = fopen(buffer, "w");
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpn_client%d_key", clientNum));
				fclose(fp);
			}

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_client%d_crt", clientNum);
			if (!nvram_is_empty(buffer)) {
				memset(buffer, 0, BUF_SIZE);
				sprintf(buffer, "/etc/openvpn/client%d/client.crt", clientNum);
				fp = fopen(buffer, "w");
				chmod(buffer, (S_IRUSR | S_IWUSR));
				fprintf(fp, "%s", getNVRAMVar("vpn_client%d_crt", clientNum));
				fclose(fp);
			}
		}
		if (userauth) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/client%d/up", clientNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_username", clientNum));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_client%d_password", clientNum));
			fclose(fp);
		}
	}
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_hmac", clientNum);
	if ((cryptMode == SECRET) || (cryptMode == TLS && nvram_get_int(buffer) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_static", clientNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/client%d/static.key", clientNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_client%d_static", clientNum));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_firewall", clientNum);
	if (!nvram_contains_word(buffer, "custom")) {
		/* Create firewall rules */
		mkdir("/etc/openvpn/fw", 0700);
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "/etc/openvpn/fw/client%d-fw.sh", clientNum);
		fp = fopen(buffer, "w");
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
		fprintf(fp, "#!/bin/sh\n");

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_fw", clientNum);
		nvi = nvram_get_int(buffer);
		fprintf(fp, "iptables -I INPUT -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -i %s -m state --state NEW -j %s\n"
		            "iptables -I FORWARD -o %s -j ACCEPT\n",
		            iface,
		            (nvi ? "DROP" : "ACCEPT"),
		            iface,
		            (nvi ? "DROP" : "ACCEPT"),
		            iface);

		if (routeMode == NAT) {
			/* Add the nat for all active bridges */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				int ret1, ret2;

				ret1 = sscanf(getNVRAMVar((i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
				ret2 = sscanf(getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
				if (ret1 == 4 && ret2 == 4) {
					fprintf(fp, "iptables -t nat -I POSTROUTING -s %d.%d.%d.%d/%s -o %s -j MASQUERADE\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3], getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i), iface);
				}
			}
		}

		/* Create firewall rules for IPv6 */
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			fprintf(fp, "ip6tables -I INPUT -i %s -m state --state NEW -j %s\n"
			            "ip6tables -I FORWARD -i %s -m state --state NEW -j %s\n"
			            "ip6tables -I FORWARD -o %s -j ACCEPT\n",
			            iface,
			            (nvi ? "DROP" : "ACCEPT"),
			            iface,
			            (nvi ? "DROP" : "ACCEPT"),
			            iface);
		}
#endif

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_client%d_rgw", clientNum);
		nvi = nvram_get_int(buffer);
		if (nvi >= OVPN_RGW_POLICY) {
			/* Disable rp_filter when in policy mode */
			fprintf(fp, "echo 0 > /proc/sys/net/ipv4/conf/%s/rp_filter\n"
			            "echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter\n",
			            iface);

#if defined(TCONFIG_BCMARM)
			modprobe("xt_set");
			modprobe("ip_set");
			modprobe("ip_set_hash_ip");
#else
			modprobe("ipt_set");
			modprobe("ip_set");
			modprobe("ip_set_iphash");
#endif

		}

		fclose(fp);

		/* Run the firewall rules */
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "/etc/openvpn/fw/client%d-fw.sh", clientNum);
		argv[0] = buffer;
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	/* In case of openvpn unexpectedly dies and leaves it added - flush tun IF, otherwise openvpn will not re-start (required by iproute2) */
	eval("/usr/sbin/ip", "addr", "flush", "dev", iface);

	/* Start the VPN client */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/vpnclient%d", clientNum);
	memset(buffer2, 0, 32);
	sprintf(buffer2, "/etc/openvpn/client%d", clientNum);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread clients on cpu 1,0 or 1,2,3,0 (in that order) */
	cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;
	snprintf(cpulist, sizeof(cpulist), "%d", (clientNum & cpu_num));

	taskset_ret = cpu_eval(NULL, cpulist, buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret)
#endif
	taskset_ret = xstart(buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Starting OpenVPN failed...");
#endif
		stop_ovpn_client(clientNum);
		return;
	}

	/* Set up cron job */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d_poll", clientNum);
	if ((nvi = nvram_get_int(buffer)) > 0) {
		/* check step value for cru minutes; values > 30 are not usefull;
		 * Example: vpn_client1_poll = 45 (minutes) leads to: 18:00 --> 18:45 --> 19:00 --> 19:45
		 */
		if (nvi > 30)
			nvi = 30;

		argv[0] = "cru";
		argv[1] = "a";
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "CheckVPNClient%d", clientNum);
		argv[2] = buffer;
		sprintf(&buffer[strlen(buffer) + 1], "*/%d * * * * service vpnclient%d start", nvi, clientNum);
		argv[3] = &buffer[strlen(buffer) + 1];
		argv[4] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d", clientNum);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_client(int clientNum)
{
	int argc;
	char *argv[7];
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnclient%d", clientNum);
	if (getpid() != 1) {
		stop_service(buffer);
		return;
	}

	/* Remove cron job */
	argv[0] = "cru";
	argv[1] = "d";
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "CheckVPNClient%d", clientNum);
	argv[2] = buffer;
	argv[3] = NULL;
	_eval(argv, NULL, 0, NULL);

	/* Stop the VPN client */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnclient%d", clientNum);
	ovpn_waitfor(buffer);

	/* NVRAM setting for device type could have changed, just try to remove both */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --rmtun --dev tap%d", (clientNum + CLIENT_IF_START));
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --rmtun --dev tun%d", (clientNum + CLIENT_IF_START));
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Remove firewall rules after VPN exit */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/fw/client%d-fw.sh", clientNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g;s/INPUT\\ [0-9]\\ /INPUT\\ /g;s/FORWARD\\ [0-9]\\ /FORWARD\\ /g;s/PREROUTING\\ [0-9]\\ /PREROUTING\\ /g;s/POSTROUTING\\ [0-9]\\ /POSTROUTING\\ /g";
	argv[3] = buffer;
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL)) {
		argv[0] = buffer;
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	/* Delete all files for this client */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "rm -rf /etc/openvpn/client%d /etc/openvpn/fw/client%d-fw.sh /etc/openvpn/vpnclient%d /etc/openvpn/dns/client%d.resolv", clientNum, clientNum, clientNum, clientNum);
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Attempt to remove directories. Will fail if not empty */
	rmdir("/etc/openvpn/fw");
	rmdir("/etc/openvpn/dns");
	rmdir("/etc/openvpn");

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_client%d", clientNum);
	allow_fastnat(buffer, 1);
	try_enabling_fastnat();
}

void start_ovpn_server(int serverNum)
{
	FILE *fp, *ccd;
	char iface[IF_SIZE];
	char buffer[BUF_SIZE];
	char buffer2[32];
	char *argv[6], *chp, *route;
	char *br_ipaddr, *br_netmask;
	int push_lan[4] = {0};
	int dont_push_active = 0;
	int argc = 0;
	int c2c = 0;
	enum { TAP, TUN } ifType = TUN;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	int nvi, ip[4], nm[4];
	long int nvl;
	int pid, i;
	int taskset_ret = 0;
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	char cpulist[2];
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
#endif

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnserver%d", serverNum);
	if (getpid() != 1) {
		start_service(buffer);
		return;
	}

	if ((pid = pidof(buffer)) >= 0)
		return;

	/* Determine interface type */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_if", serverNum);
	if (nvram_contains_word(buffer, "tap"))
		ifType = TAP;
	else if (nvram_contains_word(buffer, "tun"))
		ifType = TUN;
	else
		return;

	/* Build interface name */
	snprintf(iface, IF_SIZE, "%s%d", nvram_safe_get(buffer), serverNum+SERVER_IF_START);

	/* Determine encryption mode */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_crypt", serverNum);
	if (nvram_contains_word(buffer, "tls"))
		cryptMode = TLS;
	else if (nvram_contains_word(buffer, "secret"))
		cryptMode = SECRET;
	else if (nvram_contains_word(buffer, "custom"))
		cryptMode = CUSTOM;
	else
		return;

	/* Make sure openvpn directory exists */
	mkdir("/etc/openvpn", 0700);
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/server%d", serverNum);
	mkdir(buffer, 0700);

	/* Make sure symbolic link exists */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/vpnserver%d", serverNum);
	unlink(buffer);
	if (symlink("/usr/sbin/openvpn", buffer)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Creating symlink failed...");
#endif
		stop_ovpn_server(serverNum);
		return;
	}

	/* Make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	/* Create tap/tun interface */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --mktun --dev %s", iface);
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Creating tunnel interface failed...");
#endif
		stop_ovpn_server(serverNum);
		return;
	}

	/* Add interface to LAN bridge (TAP only) */
	if (ifType == TAP) {
		memset(buffer, 0, BUF_SIZE);
		snprintf(buffer, BUF_SIZE, "brctl addif %s %s", getNVRAMVar("vpn_server%d_br", serverNum), iface);
		for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
			syslog(LOG_WARNING, "Adding tunnel interface to bridge failed...");
#endif
			stop_ovpn_server(serverNum);
			return;
		}
	}

	/* Bring interface up */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "ifconfig %s 0.0.0.0 promisc up", iface);
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if (_eval(argv, NULL, 0, NULL)) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Bringing up tunnel interface failed...");
#endif
		stop_ovpn_server(serverNum);
		return;
	}

	/* Build and write config files */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/server%d/config.ovpn", serverNum);
	fp = fopen(buffer, "w");
	chmod(buffer, (S_IRUSR | S_IWUSR));

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_port", serverNum);
	fprintf(fp, "# Generated Configuration\n"
	            "daemon openvpn-server%d\n"
	            "port %d\n"
	            "dev %s\n"
	            "txqueuelen 1000\n"
	            "keepalive 15 60\n"
	            "verb 3\n",
	            serverNum,
	            nvram_get_int(buffer),
	            iface);

	if (cryptMode == TLS) {
		if (ifType == TUN) {
			fprintf(fp, "server %s ", getNVRAMVar("vpn_server%d_sn", serverNum));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_nm", serverNum));
		}
		else if (ifType == TAP) {
			fprintf(fp, "server-bridge");
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_dhcp", serverNum);
			if (nvram_get_int(buffer) == 0) {
				memset(buffer, 0, BUF_SIZE);
				sprintf(buffer, "vpn_server%d_br", serverNum);
				if (nvram_contains_word(buffer, "br1")) {
					br_ipaddr = nvram_get("lan1_ipaddr");
					br_netmask = nvram_get("lan1_netmask");
				}
				else if (nvram_contains_word(buffer, "br2")) {
					br_ipaddr = nvram_get("lan2_ipaddr");
					br_netmask = nvram_get("lan2_netmask");
				}
				else if (nvram_contains_word(buffer, "br3")) {
					br_ipaddr = nvram_get("lan3_ipaddr");
					br_netmask = nvram_get("lan3_netmask");
				}
				else {
					br_ipaddr = nvram_get("lan_ipaddr");
					br_netmask = nvram_get("lan_netmask");
				}

				fprintf(fp, " %s "
				            "%s ",
				            br_ipaddr,
				            br_netmask);
				fprintf(fp, "%s ", getNVRAMVar("vpn_server%d_r1", serverNum));
				fprintf(fp, "%s", getNVRAMVar("vpn_server%d_r2", serverNum));
			}
			fprintf(fp, "\n");
		}
	}
	else if (cryptMode == SECRET) {
		if (ifType == TUN) {
			fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_local", serverNum));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_remote", serverNum));
		}
	}
	fprintf(fp, "proto %s\n", getNVRAMVar("vpn_server%d_proto", serverNum)); /* full dual-stack functionality starting with OpenVPN 2.4.0 */

	/* Cipher */
	strlcpy(buffer, getNVRAMVar("vpn_server%d_ncp_ciphers", serverNum), sizeof(buffer));
	if (cryptMode == TLS) {
		if (buffer[0] != '\0')
#ifndef TCONFIG_OPTIMIZE_SIZE
			fprintf(fp, "data-ciphers %s\n", buffer);
#else
			fprintf(fp, "ncp-ciphers %s\n", buffer);
#endif
	}
#ifndef TCONFIG_OPTIMIZE_SIZE
	else {	/* SECRET/CUSTOM */
#endif
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_cipher", serverNum);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
#ifndef TCONFIG_OPTIMIZE_SIZE
	}
#endif

	/* Digest */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_digest", serverNum);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	/* Compression */
	memset(buffer, 0, BUF_SIZE);
	strlcpy(buffer, getNVRAMVar("vpn_server%d_comp", serverNum), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		if (!strcmp(buffer, "lz4") || !strcmp(buffer, "lz4-v2"))
			fprintf(fp, "compress %s\n", buffer);
		else
#endif
		     if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n");	/* Disable, but client can override if desired */
	}

	if ((nvl = atol(getNVRAMVar("vpn_server%d_reneg", serverNum))) >= 0)
		fprintf(fp, "reneg-sec %ld\n", nvl);

	if (cryptMode == TLS) {
		if (ifType == TUN) {
			/* push LANs */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				memset(buffer, 0, BUF_SIZE);
				sprintf(buffer, (i == 0 ? "vpn_server%d_plan" : "vpn_server%d_plan%d"), serverNum, i);
				if (nvram_get_int(buffer)) {
					int ret3 = 0, ret4 = 0;

					ret3 = sscanf(getNVRAMVar((i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
					ret4 = sscanf(getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
					if (ret3 == 4 && ret4 == 4) {
						fprintf(fp, "push \"route %d.%d.%d.%d %s\"\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3], getNVRAMVar((i == 0 ? "lan_netmask" : "lan%d_netmask"), i));
						push_lan[i] = 1; /* IPv4 LANX will be pushed */
					}
				}
			}
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_ccd", serverNum);
		if (nvram_get_int(buffer)) {
			fprintf(fp, "client-config-dir ccd\n");

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_c2c", serverNum);
			if ((c2c = nvram_get_int(buffer)))
				fprintf(fp, "client-to-client\n");

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_ccd_excl", serverNum);
			if (nvram_get_int(buffer))
				fprintf(fp, "ccd-exclusive\n");
			else
				fprintf(fp, "duplicate-cn\n");

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/ccd", serverNum);
			mkdir(buffer, 0700);
			chdir(buffer);

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_ccd_val", serverNum);
			strcpy(buffer, nvram_safe_get(buffer));
			chp = strtok(buffer, ">");
			while (chp != NULL) {
				nvi = strlen(chp);

				chp[strcspn(chp, "<")] = '\0';
				if (atoi(chp) == 1) {
					nvi -= strlen(chp)+1;
					chp += strlen(chp)+1;

					ccd = NULL;
					route = NULL;
					if (nvi > 0) {
						chp[strcspn(chp, "<")] = '\0';
						ccd = fopen(chp, "a");
						chmod(chp, (S_IRUSR | S_IWUSR));

						nvi -= strlen(chp) + 1;
						chp += strlen(chp) + 1;
					}
					if ((nvi > 0) && (ccd != NULL) && (strcspn(chp, "<") != strlen(chp))) {
						chp[strcspn(chp, "<")] = ' ';
						chp[strcspn(chp, "<")] = '\0';
						route = chp;
						if (strlen(route) > 1) {
							fprintf(ccd, "iroute %s\n", route);
							fprintf(fp, "route %s\n", route);
						}

						nvi -= strlen(chp) + 1;
						chp += strlen(chp) + 1;
					}
					if (ccd != NULL)
						fclose(ccd);
					if ((nvi > 0) && (route != NULL)) {
						chp[strcspn(chp, "<")] = '\0';
						if (c2c && atoi(chp) == 1 && strlen(route) > 1)
							fprintf(fp, "push \"route %s\"\n", route);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
				}
				/* Advance to next entry */
				chp = strtok(NULL, ">");
			}
		}
		else
			fprintf(fp, "duplicate-cn\n");

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_userpass", serverNum);
		if (nvram_get_int(buffer)) {
			fprintf(fp, "plugin /lib/openvpn_plugin_auth_nvram.so vpn_server%d_users_val\n"
			            "script-security 2\n"
			            "username-as-common-name\n",
			            serverNum);

			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_nocert", serverNum);
			if (nvram_get_int(buffer))
				fprintf(fp, "verify-client-cert optional\n");
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_pdns", serverNum);
		if (nvram_get_int(buffer)) {
			if (nvram_safe_get("wan_domain")[0] != '\0')
				fprintf(fp, "push \"dhcp-option DOMAIN %s\"\n", nvram_safe_get("wan_domain"));
			if ((nvram_safe_get("wan_wins")[0] != '\0' && strcmp(nvram_safe_get("wan_wins"), "0.0.0.0") != 0))
				fprintf(fp, "push \"dhcp-option WINS %s\"\n", nvram_safe_get("wan_wins"));

			/* check if LANX will be pushed --> if YES, push the suitable DNS Server address */
			for (i = 0; i < BRIDGE_COUNT; i++) {
				if (push_lan[i] == 1) { /* push IPv4 LANx DNS */
					memset(buffer, 0, BUF_SIZE);
					sprintf(buffer, (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
					dont_push_active = 1;
				}
			}
			/* no LANx will be pushed, push only one active DNS */
			/* check what LAN is active before push DNS */
			if (dont_push_active == 0) {
				for (i = 0; i < BRIDGE_COUNT; i++) {
					memset(buffer, 0, BUF_SIZE);
					sprintf(buffer, (i == 0 ? "lan_ipaddr" : "lan%d_ipaddr"), i);
					if (strcmp(nvram_safe_get(buffer), "") != 0) {
						fprintf(fp, "push \"dhcp-option DNS %s\"\n", nvram_safe_get(buffer));
						break;
					}
				}
			}
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_rgw", serverNum);
		if (nvram_get_int(buffer)) {
			if (ifType == TAP)
				fprintf(fp, "push \"route-gateway %s\"\n", nvram_safe_get("lan_ipaddr"));
			fprintf(fp, "push \"redirect-gateway def1\"\n");
		}

		nvi = atoi(getNVRAMVar("vpn_server%d_hmac", serverNum));
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_static", serverNum);
		if (!nvram_is_empty(buffer) && nvi >= 0) {
			if (nvi == 3)
				fprintf(fp, "tls-crypt static.key");
#ifndef TCONFIG_OPTIMIZE_SIZE
			else if (nvi == 4)
				fprintf(fp, "tls-crypt-v2 static.key");
#endif
			else
				fprintf(fp, "tls-auth static.key");

			if (nvi < 2)
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_ca", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "ca ca.crt\n");
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_dh", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "dh dh.pem\n");
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_crt", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "cert server.crt\n");
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_crl", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "crl crl.pem\n");
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_key", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "key server.key\n");
	}
	else if (cryptMode == SECRET) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_static", serverNum);
		if (!nvram_is_empty(buffer))
			fprintf(fp, "secret static.key\n");
	}
	fprintf(fp, "status-version 2\n"
	            "status status 10\n\n" /* Update status file every 10 sec */
	            "# Custom Configuration\n"
	            "%s",
	            getNVRAMVar("vpn_server%d_custom", serverNum));

	fclose(fp);

	/* Write certification and key files */
	if (cryptMode == TLS) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_ca", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/ca.crt", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_ca", serverNum));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_key", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/server.key", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_key", serverNum));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_crt", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/server.crt", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_crt", serverNum));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_crl", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/crl.pem", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_crl", serverNum));
			fclose(fp);
		}

		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_dh", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/dh.pem", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_dh", serverNum));
			fclose(fp);
		}
	}
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_hmac", serverNum);
	if ((cryptMode == SECRET) || (cryptMode == TLS && nvram_get_int(buffer) >= 0)) {
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_static", serverNum);
		if (!nvram_is_empty(buffer)) {
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "/etc/openvpn/server%d/static.key", serverNum);
			fp = fopen(buffer, "w");
			chmod(buffer, (S_IRUSR | S_IWUSR));
			fprintf(fp, "%s", getNVRAMVar("vpn_server%d_static", serverNum));
			fclose(fp);
		}
	}

	/* Handle firewall rules if appropriate */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_firewall", serverNum);
	if (!nvram_contains_word(buffer, "custom")) {
		/* Create firewall rules */
		mkdir("/etc/openvpn/fw", 0700);
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "/etc/openvpn/fw/server%d-fw.sh", serverNum);
		fp = fopen(buffer, "w");
		chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));
		fprintf(fp, "#!/bin/sh\n");
		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpn_server%d_proto", serverNum), BUF_SIZE);
		fprintf(fp, "iptables -t nat -I PREROUTING -p %s ", strtok(buffer, "-"));
		fprintf(fp, "--dport %d -j ACCEPT\n", atoi(getNVRAMVar("vpn_server%d_port", serverNum)));
		memset(buffer, 0, BUF_SIZE);
		strncpy(buffer, getNVRAMVar("vpn_server%d_proto", serverNum), BUF_SIZE);
		fprintf(fp, "iptables -I INPUT -p %s ", strtok(buffer, "-"));
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_port", serverNum);
		fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(buffer));
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "vpn_server%d_firewall", serverNum);
		if (!nvram_contains_word(buffer, "external")) {
			fprintf(fp, "iptables -I INPUT -i %s -j ACCEPT\n"
			            "iptables -I FORWARD -i %s -j ACCEPT\n",
			            iface,
			            iface);
		}

		/* Create firewall rules for IPv6 */
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			strncpy(buffer, getNVRAMVar("vpn_server%d_proto", serverNum), BUF_SIZE);
			fprintf(fp, "ip6tables -I INPUT -p %s ", strtok(buffer, "-"));
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_port", serverNum);
			fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(buffer));
			memset(buffer, 0, BUF_SIZE);
			sprintf(buffer, "vpn_server%d_firewall", serverNum);
			if (!nvram_contains_word(buffer, "external")) {
				fprintf(fp, "ip6tables -I INPUT -i %s -j ACCEPT\n"
				            "ip6tables -I FORWARD -i %s -j ACCEPT\n",
				            iface,
				            iface);
			}
		}
#endif

		fclose(fp);

		/* Run the firewall rules */
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "/etc/openvpn/fw/server%d-fw.sh", serverNum);
		argv[0] = buffer;
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	/* Start the VPN server */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/vpnserver%d", serverNum);
	memset(buffer2, 0, 32);
	sprintf(buffer2, "/etc/openvpn/server%d", serverNum);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	/* Spread servers on cpu 1,0 or 1,2 (in that order) */
	cpu_num = sysconf(_SC_NPROCESSORS_CONF) - 1;
	if (cpu_num < 0)
		cpu_num = 0;
	snprintf(cpulist, sizeof(cpulist), "%d", (serverNum & cpu_num));

	taskset_ret = cpu_eval(NULL, cpulist, buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret)
#endif
	taskset_ret = xstart(buffer, "--cd", buffer2, "--config", "config.ovpn");

	if (taskset_ret) {
#ifndef TCONFIG_OPTIMIZE_SIZE
		syslog(LOG_WARNING, "Starting VPN instance failed...");
#endif
		stop_ovpn_client(serverNum);
		return;
	}

	/* Set up cron job */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d_poll", serverNum);
	if ((nvi = nvram_get_int(buffer)) > 0) {
		/* check step value for cru minutes; values > 30 are not usefull;
		 * Example: vpn_server1_poll = 45 (minutes) leads to: 18:00 --> 18:45 --> 19:00 --> 19:45
		 */
		if (nvi > 30)
			nvi = 30;

		argv[0] = "cru";
		argv[1] = "a";
		memset(buffer, 0, BUF_SIZE);
		sprintf(buffer, "CheckVPNServer%d", serverNum);
		argv[2] = buffer;
		sprintf(&buffer[strlen(buffer) + 1], "*/%d * * * * service vpnserver%d start", nvi, serverNum);
		argv[3] = &buffer[strlen(buffer) + 1];
		argv[4] = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d", serverNum);
	allow_fastnat(buffer, 0);
	try_enabling_fastnat();
}

void stop_ovpn_server(int serverNum)
{
	int argc;
	char *argv[9];
	char buffer[BUF_SIZE];

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnserver%d", serverNum);
	if (getpid() != 1) {
		stop_service(buffer);
		return;
	}

	/* Remove cron job */
	argv[0] = "cru";
	argv[1] = "d";
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "CheckVPNServer%d", serverNum);
	argv[2] = buffer;
	argv[3] = NULL;
	_eval(argv, NULL, 0, NULL);

	/* Stop the VPN server */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpnserver%d", serverNum);
	ovpn_waitfor(buffer);

	/* NVRAM setting for device type could have changed, just try to remove both */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --rmtun --dev tap%d", (serverNum + SERVER_IF_START));
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "openvpn --rmtun --dev tun%d", (serverNum + SERVER_IF_START));
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Remove firewall rules */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "/etc/openvpn/fw/server%d-fw.sh", serverNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g;s/INPUT\\ [0-9]\\ /INPUT\\ /g;s/FORWARD\\ [0-9]\\ /FORWARD\\ /g;s/PREROUTING\\ [0-9]\\ /PREROUTING\\ /g;s/POSTROUTING\\ [0-9]\\ /POSTROUTING\\ /g";
	argv[3] = buffer;
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL)) {
		argv[0] = buffer;
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}

	/* Delete all files for this server */
	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "rm -rf /etc/openvpn/server%d /etc/openvpn/fw/server%d-fw.sh /etc/openvpn/vpnserver%d", serverNum, serverNum, serverNum);
	for (argv[argc = 0] = strtok(buffer, " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	/* Attempt to remove directories.  Will fail if not empty */
	rmdir("/etc/openvpn/fw");
	rmdir("/etc/openvpn");

	memset(buffer, 0, BUF_SIZE);
	sprintf(buffer, "vpn_server%d", serverNum);
	allow_fastnat(buffer, 1);
	try_enabling_fastnat();
}

void start_ovpn_eas()
{
	char buffer[16], *cur;
	int nums[OVPN_CLIENT_MAX], i;

	if ((strlen(nvram_safe_get("vpn_server_eas")) == 0) && (strlen(nvram_safe_get("vpn_client_eas")) == 0))
		return;

	/* wait for time sync for a while */
	i = 10;
	while (time(0) < Y2K && i--) {
		sleep(1);
	}

	/* Parse and start servers */
	strlcpy(buffer, nvram_safe_get("vpn_server_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnserver%d", nums[i]);

		if (pidof(buffer) >= 0)
			stop_ovpn_server(nums[i]);

		start_ovpn_server(nums[i]);
	}

	/* Parse and start clients */
	strlcpy(buffer, nvram_safe_get("vpn_client_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnclient%d", nums[i]);

		if (pidof(buffer) >= 0)
			stop_ovpn_client(nums[i]);

		start_ovpn_client(nums[i]);
	}
}

void stop_ovpn_eas()
{
	char buffer[16], *cur;
	int nums[OVPN_CLIENT_MAX], i;

	/* Parse and stop servers */
	strlcpy(buffer, nvram_safe_get("vpn_server_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_SERVER_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_SERVER_MAX); i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnserver%d", nums[i]);

		if (pidof(buffer) >= 0)
			stop_ovpn_server(nums[i]);
	}

	/* Parse and stop clients */
	strlcpy(buffer, nvram_safe_get("vpn_client_eas"), sizeof(buffer));

	i = 0;
	for (cur = strtok(buffer, ","); (cur != NULL) && (i <= OVPN_CLIENT_MAX); cur = strtok(NULL, ","))
		nums[i++] = atoi(cur);

	nums[i] = 0;
	for (i = 0; (nums[i] > 0) && (nums[i] <= OVPN_CLIENT_MAX); i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnclient%d", nums[i]);

		if (pidof(buffer) >= 0)
			stop_ovpn_client(nums[i]);
	}
}

void stop_ovpn_all()
{
	char buffer[16];
	int i;

	/* Stop servers */
	for (i = 1; i <= OVPN_SERVER_MAX; i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnserver%d", i);
		if (pidof(buffer) >= 0)
			stop_ovpn_server(i);
	}

	/* Stop clients */
	for (i = 1; i <= OVPN_CLIENT_MAX; i++) {
		memset(buffer, 0, 16);
		sprintf(buffer, "vpnclient%d", i);
		if (pidof(buffer) >= 0)
			stop_ovpn_client(i);
	}

	/* Remove tunnel interface module */
	modprobe_r("tun");
}

void run_ovpn_firewall_scripts()
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	char *argv[8];

	if (chdir("/etc/openvpn/fw"))
		return;

	dir = opendir("/etc/openvpn/fw");

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		/* Remove existing firewall rules if they exist */
		argv[0] = "sed";
		argv[1] = "s/-A/-D/g;s/-I/-D/g;s/INPUT\\ [0-9]\\ /INPUT\\ /g;s/FORWARD\\ [0-9]\\ /FORWARD\\ /g;s/PREROUTING\\ [0-9]\\ /PREROUTING\\ /g;s/POSTROUTING\\ [0-9]\\ /POSTROUTING\\ /g";
		argv[2] = fn;
		argv[3] = ">";
		argv[4] = "/etc/openvpn/fw/clear-fw-tmp.sh";
		argv[5] = NULL;
		if (!_eval(argv, NULL, 0, NULL)) {
			argv[0] = "/etc/openvpn/fw/clear-fw-tmp.sh";
			argv[1] = NULL;
			_eval(argv, NULL, 0, NULL);
		}
		unlink("/etc/openvpn/fw/clear-fw-tmp.sh");

		/* Add firewall rules */
		argv[0] = "/bin/sh";
		argv[1] = fn;
		argv[2] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	closedir(dir);
}

void write_ovpn_dnsmasq_config(FILE* f)
{
	char nv[16];
	char buf[24];
	char *pos, *fn, ch;
	int cur;
	DIR *dir;
	struct dirent *file;

	strlcpy(buf, nvram_safe_get("vpn_server_dns"), sizeof(buf));
	for (pos = strtok(buf, ","); pos != NULL; pos = strtok(NULL, ",")) {
		cur = atoi(pos);
		if (cur) {
			snprintf(nv, sizeof(nv), "vpn_server%d_if", cur);
			fprintf(f, "interface=%s%d\n", nvram_safe_get(nv), (SERVER_IF_START + cur));
		}
	}

	if ((dir = opendir("/etc/openvpn/dns")) != NULL) {
		while ((file = readdir(dir)) != NULL) {
			fn = file->d_name;

			if (fn[0] == '.')
				continue;

			if (sscanf(fn, "client%d.resol%c", &cur, &ch) == 2) {
				snprintf(buf, sizeof(buf), "vpn_client%d_adns", cur);
				if (nvram_get_int(buf) == 2) {
					fprintf(f, "strict-order\n");
					break;
				}
			}

			if (sscanf(fn, "client%d.con%c", &cur, &ch) == 2)
				fappend(f, fn);
		}
		closedir(dir);
	}
}

int write_ovpn_resolv(FILE* f)
{
	DIR *dir;
	struct dirent *file;
	char *fn, ch, num, buf[24];
	FILE *dnsf;
	int exclusive = 0;
	int adns = 0;

	if (chdir("/etc/openvpn/dns"))
		return 0;

	dir = opendir("/etc/openvpn/dns");

	while ((file = readdir(dir)) != NULL) {
		fn = file->d_name;

		if (fn[0] == '.')
			continue;

		if (sscanf(fn, "client%c.resol%c", &num, &ch) == 2) {
			snprintf(buf, sizeof(buf), "vpn_client%c_adns", num);
			adns = nvram_get_int(buf);
			if ((dnsf = fopen(fn, "r")) == NULL)
				continue;

			fappend(f, fn);

			if (adns == 3)
				exclusive = 1;
		}
	}
	closedir(dir);

	return exclusive;
}
