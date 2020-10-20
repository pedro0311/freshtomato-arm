/*
 * nocat.c
 *
 * Copyright (C) 2009 zd <tomato@winddns.cn>
 * Copyright (C) 2011 Modifications for K2.6 Victek, Roadkill 
 *
 * $Id:
 */
#include <stdlib.h>
#include <rc.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>

#define NOCAT_CONF	"/tmp/etc/nocat.conf"


int build_nocat_conf( void )
{
	char *p;
	FILE *fp;


	if (!(fp = fopen(NOCAT_CONF, "w"))) {
		perror(NOCAT_CONF);
		return errno;
	}

	/*
	 * settings that need to be set based on router configurations 
	 * Autodetected on the device: lan_ifname & NC_Iface variable
	 */
	fprintf(fp,
		"#\n"
		"ExternalDevice\t%s\n"
		"RouteOnly\t%s\n",
		nvram_safe_get("wan_iface"),
		"1");

	if (nvram_match("NC_BridgeLAN", "br0")) {
		fprintf(fp, "InternalDevice\t%s\n", nvram_safe_get("lan_ifname"));
		fprintf(fp, "GatewayAddr\t%s\n", nvram_safe_get("lan_ipaddr"));
	}
	if (nvram_match("NC_BridgeLAN", "br1")) {
		fprintf(fp, "InternalDevice\t%s\n", nvram_safe_get("lan1_ifname"));
		fprintf(fp, "GatewayAddr\t%s\n", nvram_safe_get("lan1_ipaddr"));
	}
	if (nvram_match("NC_BridgeLAN", "br2"))
	{
		fprintf(fp, "InternalDevice\t%s\n", nvram_safe_get("lan2_ifname"));
		fprintf(fp, "GatewayAddr\t%s\n", nvram_safe_get("lan2_ipaddr"));
	}
	if (nvram_match("NC_BridgeLAN", "br3")) {
		fprintf(fp, "InternalDevice\t%s\n", nvram_safe_get("lan3_ifname"));
		fprintf(fp, "GatewayAddr\t%s\n", nvram_safe_get("lan3_ipaddr"));
	}

	fprintf(fp, "GatewayMAC\t%s\n", nvram_safe_get("et0macaddr"));

	/*
	 * These are user defined, eventually via the web page 
	 */
	if ((p = nvram_get("NC_Verbosity")) == NULL) p = "2";
	fprintf(fp, "Verbosity\t%s\n", p);

	if ((p = nvram_get("NC_GatewayName")) == NULL) p = "Tomato RAF Portal";
	fprintf(fp, "GatewayName\t%s\n", p);

	if ((p = nvram_get("NC_GatewayPort")) == NULL) p = "5280";
	fprintf(fp, "GatewayPort\t%s\n", p);

	if ((p = nvram_get("NC_Password")) == NULL) p = "";
	fprintf(fp, "GatewayPassword\t%s\n", p);

	if ((p = nvram_get("NC_GatewayMode")) == NULL) p = "Open";
	fprintf(fp, "GatewayMode\t%s\n", p);

	if ((p = nvram_get("NC_DocumentRoot")) == NULL) p = "/tmp/splashd";
	fprintf(fp, "DocumentRoot\t%s\n", p);

	if (nvram_invmatch("NC_SplashURL", "")) {
		fprintf(fp, "SplashURL\t%s\n", nvram_safe_get("NC_SplashURL"));
		fprintf(fp, "SplashURLTimeout\t%s\n", nvram_safe_get("NC_SplashURLTimeout"));
	}

	/*
	 * Open-mode and common options 
	 */
	fprintf(fp,
		"LeaseFile\t%s\n"
		"FirewallPath\t%s\n"
		"ExcludePorts\t%s\n"
		"IncludePorts\t%s\n"
		"AllowedWebHosts\t%s %s\n"
		"MACWhiteList\t%s\n"			/* MACWhiteList to ignore given machines or routers on the local net (e.g. routers with an alternate Auth) */
		"AnyDNS\t%s\n"				/* AnyDNS to pass through any client-defined servers */
		"HomePage\t%s\n"
		"PeerCheckTimeout\t%s\n",
		"/tmp/nocat.leases",
		"/usr/libexec/nocat/",
		nvram_safe_get("NC_ExcludePorts"),
		nvram_safe_get("NC_IncludePorts"),
		nvram_safe_get("lan_ipaddr"),
		nvram_safe_get("NC_AllowedWebHosts"),
		nvram_safe_get("NC_MACWhiteList"),
		"1",
		nvram_safe_get("NC_HomePage"),
		nvram_safe_get("NC_PeerChecktimeout"));

	if ((p = nvram_get("NC_ForcedRedirect")) == NULL) p = "0";
	fprintf(fp, "ForcedRedirect\t%s\n", p);

	if ((p = nvram_get("NC_IdleTimeout")) == NULL) p = "0";
	fprintf(fp, "IdleTimeout\t%s\n", p);

	if ((p = nvram_get("NC_MaxMissedARP")) == NULL) p = "5";
	fprintf(fp, "MaxMissedARP\t%s\n", p);

	if ((p = nvram_get("NC_LoginTimeout")) == NULL) p = "6400";
	fprintf(fp, "LoginTimeout\t%s\n", p);

	if ((p = nvram_get("NC_RenewTimeout")) == NULL) p = "0";
	fprintf(fp, "RenewTimeout\t%s\n", p);

	fclose(fp);

	fprintf(stderr, "Wrote: %s\n", NOCAT_CONF);

	return 0;
}

void start_nocat(void)
{
	FILE *fp;
	char splashfile[255];
	char logofile[255];
	char iconfile[255];
	char cpcmd[255];
	char *p;

	stop_nocat();

	if (!nvram_match("NC_enable", "1") || !nvram_match("mwan_num", "1")) {
		return;
	}

	build_nocat_conf();

	if ((p = nvram_get("NC_DocumentRoot")) == NULL) p = "/tmp/splashd";
	sprintf(splashfile, "%s/splash.html", p);
	sprintf(logofile, "%s/style.css", p);
	sprintf(iconfile, "%s/favicon.ico", p);

	if (!f_exists(splashfile)) {
		nvram_get_file("NC_SplashFile", splashfile, 8192);
		if (!f_exists(splashfile)) {
			sprintf(cpcmd, "cp /www/splash.html %s", splashfile);
			system(cpcmd);
			sprintf(cpcmd, "cp /www/style.css  %s", logofile);
			system(cpcmd);
			sprintf(cpcmd, "cp /www/favicon.ico  %s", iconfile);
			system(cpcmd);
		}
	}

	if (!(fp = fopen( "/tmp/start_splashd.sh", "w"))) {
		perror("/tmp/start_splashd.sh");
		return;
	}
	
//	if (!pidof("splashd") > 0 && (fp = fopen("/tmp/var/lock/splashd.lock", "r"))) {
//		unlink("/tmp/var/lock/splashd.lock");
//	}

	fprintf(fp,
		"#!/bin/sh\n"
		"LOGGER=logger\n"
		"LOCK_FILE=/tmp/var/lock/splashd.lock\n"
		"if [ -f $LOCK_FILE ]; then\n"
		"	$LOGGER \"Captive Portal halted (0), other process starting.\" \n"
		"	exit\n"
		"fi\n"
		"echo \"TOMATO_RAF\" > $LOCK_FILE\n"
		"sleep 20\n"
		"$LOGGER \"splashd : Captive Portal Splash Daemon successfully started\" \n"
		"echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse\n"
		"/usr/sbin/splashd >> /tmp/nocat.log 2>&1 &\n"
		"sleep 2\n"
		"echo 0 > /proc/sys/net/ipv4/tcp_tw_reuse\n"
		"rm $LOCK_FILE\n");
	fclose(fp);

	chmod("/tmp/start_splashd.sh", 0700);
	xstart("/tmp/start_splashd.sh");

	return;
}

void stop_nocat(void)
{
	if (pidof("splashd") > 0) {
		syslog(LOG_INFO, "splashd : Captive Portal Splash daemon successfully stopped\n");
		killall_tk_period_wait("splashd", 50);
		eval("/usr/libexec/nocat/uninitialize.fw");
		system("rm /tmp/nocat.leases\n");
		system("rm /tmp/start_splashd.sh\n");
		system("rm /tmp/nocat.log\n");

	}
	return;
}

void reset_nocat(void)
{
	if (pidof("splashd") > 0) {
		syslog(LOG_INFO, "splashd : Reseting splashd firewall rules\n");
		killall("splashd", SIGUSR1);
	}
	return;
}
