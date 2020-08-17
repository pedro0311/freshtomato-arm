/*
 * tor.c
 *
 * Copyright (C) 2011 shibby
 *
 */

#include <rc.h>
#include <sys/stat.h>


void start_tor(void) {
	FILE *fp;
	char *ip;

	/* only if enabled */
	if (nvram_match("tor_enable", "1")) {

		if (nvram_match("tor_solve_only", "1"))
			/* dnsmasq uses this IP for nameserver to resolv .onion/.exit domains */
			ip = nvram_safe_get("lan_ipaddr");
		else {
			if (nvram_match("tor_iface", "br0"))      { ip = nvram_safe_get("lan_ipaddr");  }
			else if (nvram_match("tor_iface", "br1")) { ip = nvram_safe_get("lan1_ipaddr"); }
			else if (nvram_match("tor_iface", "br2")) { ip = nvram_safe_get("lan2_ipaddr"); }
			else if (nvram_match("tor_iface", "br3")) { ip = nvram_safe_get("lan3_ipaddr"); }
			else                                      { ip = nvram_safe_get("lan_ipaddr");  }
		}

		/* writing data to file */
		if (!(fp = fopen( "/etc/tor.conf", "w"))) {
			perror("/etc/tor.conf");
			return;
		}
		/* localhost ports, NoPreferIPv6Automap doesn't matter when applied only to DNSPort, but works fine with SocksPort */
		fprintf(fp, "SocksPort %d NoPreferIPv6Automap\n"
		            "AutomapHostsOnResolve 1\n"		/* .exit/.onion domains support for LAN clients */
		            "VirtualAddrNetworkIPv4 172.16.0.0/12\n"
		            "VirtualAddrNetworkIPv6 [FC00::]/7\n"
		            "AvoidDiskWrites 1\n"
		            "RunAsDaemon 1\n"
		            "Log notice syslog\n"
		            "DataDirectory %s\n"
		            "TransPort %s:%s\n"
		            "DNSPort %s:%s\n"
		            "User nobody\n"
		            "%s\n",
		            nvram_get_int("tor_socksport"),
		            nvram_safe_get("tor_datadir"),
		            ip,
		            nvram_safe_get("tor_transport"),
		            ip,
		            nvram_safe_get("tor_dnsport"),
		            nvram_safe_get("tor_custom"));

		fclose(fp);

		chmod("/etc/tor.conf", 0644);
		chmod("/dev/null", 0666);

		mkdir(nvram_safe_get("tor_datadir"), 0777);

		xstart("chown", "nobody:nobody", nvram_safe_get("tor_datadir"));

		xstart("tor", "-f", "/etc/tor.conf");
	}
}

void stop_tor(void) {
	killall("tor", SIGTERM);
}
