/*

	Copyright (C) 2014 Lance Fredrickson
	lancethepants@gmail.com

*/


#include "rc.h"

#define BUF_SIZE 256
#define TINC_RSA_KEY		"/etc/tinc/rsa_key.priv"
#define TINC_PRIV_KEY		"/etc/tinc/ed25519_key.priv"
#define TINC_CONF		"/etc/tinc/tinc.conf"
#define TINC_HOSTS		"/etc/tinc/hosts"
#define TINC_UP_SCRIPT		"/etc/tinc/tinc-up"
#define TINC_DOWN_SCRIPT	"/etc/tinc/tinc-down"
#define TINC_FW_SCRIPT		"/etc/tinc/tinc-fw.sh"
#define TINC_HOSTUP_SCRIPT	"/etc/tinc/host-up"
#define TINC_HOSTDOWN_SCRIPT	"/etc/tinc/host-down"
#define TINC_SUBNETUP_SCRIPT	"/etc/tinc/subnet-up"
#define TINC_SUBNETDOWN_SCRIPT	"/etc/tinc/subnet-down"


void start_tinc(void)
{
	char *nv, *nvp, *b;
	const char *connecto, *name, *address, *port, *compression, *subnet, *rsa, *ed25519, *custom, *tinc_tmp_value;
	char buffer[BUF_SIZE];
	char cru[128];
	FILE *fp, *hp;
	int nvi;

	/* Don't try to start tinc if it is already running */
	if (pidof("tincd") >= 0)
		return;

	/* create tinc directories */
	mkdir("/etc/tinc", 0700);
	mkdir(TINC_HOSTS, 0700);

	/* write private rsa key */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_private_rsa"), "") != 0) {
		if (!(fp = fopen(TINC_RSA_KEY, "w"))) {
			perror(TINC_RSA_KEY);
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_RSA_KEY, 0600);
	}

	/* write private ed25519 key */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_private_ed25519"), "") != 0) {
		if (!(fp = fopen(TINC_PRIV_KEY, "w"))) {
			perror(TINC_PRIV_KEY);
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_PRIV_KEY, 0600);
	}

	/* create tinc.conf */
	if (!(fp = fopen(TINC_CONF, "w"))) {
		perror(TINC_CONF);
		return;
	}

	fprintf(fp, "Name = %s\n"
	            "Interface = tinc\n"
	            "DeviceType = %s\n",
	            nvram_safe_get("tinc_name"),
	            nvram_safe_get("tinc_devicetype"));

	if (nvram_match("tinc_devicetype", "tun"))
		fprintf(fp, "Mode = router\n");
	else if (nvram_match("tinc_devicetype", "tap"))
		fprintf(fp, "Mode = %s\n", nvram_safe_get("tinc_mode"));

	/* create tinc host files */
	nvp = nv = strdup(nvram_safe_get("tinc_hosts"));
	if (!nv)
		return;

	while ((b = strsep(&nvp, ">")) != NULL) {

		if (vstrsep(b, "<", &connecto, &name, &address, &port, &compression, &subnet, &rsa, &ed25519, &custom) != 9)
			continue;

		memset(buffer, 0, (BUF_SIZE));
		sprintf(buffer, TINC_HOSTS"/%s", name);
		if (!(hp = fopen(buffer, "w"))) {
			perror(buffer);
			return;
		}

		/* write Connecto's to tinc.conf, excluding the host system if connecto is enabled */
		if ((strcmp(connecto, "1") == 0) && (strcmp(nvram_safe_get("tinc_name"), name) != 0))
			fprintf(fp, "ConnectTo = %s\n", name);

		if (strcmp(rsa, "") != 0)
			fprintf(hp, "%s\n", rsa);

		if (strcmp( ed25519, "") != 0)
			fprintf(hp, "%s\n", ed25519);

		if (strcmp(address, "") != 0)
			fprintf(hp, "Address = %s\n", address);

		if (strcmp(subnet, "") != 0)
			fprintf(hp, "Subnet = %s\n", subnet);

		if (strcmp(compression, "") != 0)
			fprintf(hp, "Compression = %s\n", compression);

		if (strcmp(port, "") != 0)
			fprintf(hp, "Port = %s\n", port);

		if (strcmp(custom, "") != 0)
			fprintf(hp, "%s\n", custom);

		fclose(hp);

		/* generate tinc-up and firewall scripts */
		if (strcmp(nvram_safe_get("tinc_name"), name)  == 0) {
			/* create tinc-up script if this is the host system */
			if (!(hp = fopen(TINC_UP_SCRIPT, "w"))) {
				perror(TINC_UP_SCRIPT);
				return;
			}

			fprintf(hp, "#!/bin/sh\n");

			/* Determine whether automatically generate tinc-up, or use manually supplied script */
			if (!nvram_match("tinc_manual_tinc_up", "1")) {

				if (nvram_match("tinc_devicetype", "tun"))
					fprintf(hp, "ifconfig $INTERFACE %s netmask %s\n", nvram_safe_get("lan_ipaddr"), nvram_safe_get("tinc_vpn_netmask"));
				else if (nvram_match("tinc_devicetype", "tap")) {
					fprintf(hp, "brctl addif %s $INTERFACE\n", nvram_safe_get("lan_ifname"));
					fprintf(hp, "ifconfig $INTERFACE 0.0.0.0 promisc up\n");
				}
			}
			else
				fprintf(hp, "%s\n", nvram_safe_get("tinc_tinc_up"));

			fclose(hp);
			chmod(TINC_UP_SCRIPT, 0744);

			/* Create firewall script */
			if (!(hp = fopen(TINC_FW_SCRIPT, "w"))) {
				perror(TINC_FW_SCRIPT);
				return;
			}

			fprintf(hp, "#!/bin/sh\n");

			if (!nvram_match("tinc_manual_firewall", "2")) {
				if (strcmp(port, "") == 0)
					port = "655";

				fprintf(hp, "iptables -I INPUT -p udp --dport %s -j ACCEPT\n"
				            "iptables -I INPUT -p tcp --dport %s -j ACCEPT\n"
				            "iptables -I INPUT -i tinc -j ACCEPT\n"
				            "iptables -I FORWARD -i tinc -j ACCEPT\n",
				            port,
				            port);

#ifdef TCONFIG_IPV6
				if (ipv6_enabled())
					fprintf(hp, "\n"
					            "ip6tables -I INPUT -p udp --dport %s -j ACCEPT\n"
					            "ip6tables -I INPUT -p tcp --dport %s -j ACCEPT\n"
					            "ip6tables -I INPUT -i tinc -j ACCEPT\n"
					            "ip6tables -I FORWARD -i tinc -j ACCEPT\n",
					            port,
					            port);
#endif
			}

			if (!nvram_match("tinc_manual_firewall", "0")) {
				fprintf(hp, "\n");
				fprintf(hp, "%s\n", nvram_safe_get("tinc_firewall"));
			}

			fclose(hp);
			chmod(TINC_FW_SCRIPT, 0744);
		}
	}

	/* Write tinc.conf custom configuration */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_custom"), "") != 0)
		fprintf(fp, "%s\n", tinc_tmp_value);

	fclose(fp);
	free(nv);

	/* write tinc-down script */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_tinc_down"), "") != 0) {
		if (!(fp = fopen(TINC_DOWN_SCRIPT, "w"))) {
			perror(TINC_DOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_DOWN_SCRIPT, 0744);
	}

	/* write host-up */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_host_up"), "") != 0) {
		if (!(fp = fopen(TINC_HOSTUP_SCRIPT, "w"))) {
			perror(TINC_HOSTUP_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod(TINC_HOSTUP_SCRIPT, 0744);
	}

	/* write host-down */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_host_down"), "") != 0) {
		if (!(fp = fopen(TINC_HOSTDOWN_SCRIPT, "w"))) {
			perror(TINC_HOSTDOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_HOSTDOWN_SCRIPT, 0744);
	}

	/* write subnet-up */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_subnet_up"), "") != 0) {
		if (!(fp = fopen(TINC_SUBNETUP_SCRIPT, "w"))) {
			perror(TINC_SUBNETUP_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_SUBNETUP_SCRIPT, 0744);
	}

	/* write subnet-down */
	if (strcmp(tinc_tmp_value = nvram_safe_get("tinc_subnet_down"), "") != 0) {
		if (!(fp = fopen(TINC_SUBNETDOWN_SCRIPT, "w"))) {
			perror(TINC_SUBNETDOWN_SCRIPT);
			return;
		}
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "%s\n", tinc_tmp_value);
		fclose(fp);
		chmod(TINC_SUBNETDOWN_SCRIPT, 0744);
	}

	/* Make sure module is loaded */
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	run_tinc_firewall_script();
	xstart("/usr/sbin/tinc", "start");

	if ((nvi = nvram_get_int("tinc_poll")) > 0) {
		memset(cru, 0, 128);
		sprintf(cru, "*/%d * * * * service tinc start", nvi);
		eval("cru", "a", "CheckTincDaemon", cru);
	}
}

void stop_tinc(void)
{
	killall("tincd", SIGTERM);
	system("/bin/sed -i \'s/-A/-D/g;s/-I/-D/g\' "TINC_FW_SCRIPT);
	run_tinc_firewall_script();
	system("/bin/rm -rf /etc/tinc");
	eval("cru", "d", "CheckTincDaemon");
}

void run_tinc_firewall_script(void)
{
	FILE *fp;

	if ((fp = fopen(TINC_FW_SCRIPT, "r"))) {
		fclose(fp);
		system(TINC_FW_SCRIPT);
	}
}

void start_tinc_wanup(void)
{
	if (nvram_match("tinc_wanup", "1"))
		start_tinc();
}
