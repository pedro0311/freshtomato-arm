/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/
/*

	wificonf, OpenWRT
	Copyright (C) 2005 Felix Fietkau <nbd@vd-s.ath.cx>
	
*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#ifndef UU_INT
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#endif

#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <wlutils.h>
#include <bcmparams.h>
#include <wlioctl.h>

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION >= 108
#include <etioctl.h>
#else
#include <etsockio.h>
#endif

#ifdef TCONFIG_BCMWL6
#include <d11.h>
#define WLCONF_PHYTYPE2STR(phy)	((phy) == PHY_TYPE_A ? "a" : \
				 (phy) == PHY_TYPE_B ? "b" : \
				 (phy) == PHY_TYPE_LP ? "l" : \
				 (phy) == PHY_TYPE_G ? "g" : \
				 (phy) == PHY_TYPE_SSN ? "s" : \
				 (phy) == PHY_TYPE_HT ? "h" : \
				 (phy) == PHY_TYPE_AC ? "v" : \
				 (phy) == PHY_TYPE_LCN ? "c" : "n")
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"network_debug"


void restart_wl(void);
void stop_lan_wl(void);
void start_lan_wl(void);

#ifdef TCONFIG_BCMWL6
void wlconf_pre(void)
{
	int unit = 0;
	char word[128], *next;
	char tmp[128], prefix[] = "wlXXXXXXXXXX_";
	char buf[16] = {0};
	wlc_rev_info_t rev;

	foreach (word, nvram_safe_get("wl_ifnames"), next) {

		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

		/* for TxBeamforming: get corerev for TxBF check */
		wl_ioctl(word, WLC_GET_REVINFO, &rev, sizeof(rev));
		snprintf(buf, sizeof(buf), "%d", rev.corerev);
		nvram_set(strcat_r(prefix, "corerev", tmp), buf);

		if (rev.corerev < 40) { /* TxBF unsupported - turn off and hide options (at the GUI) */
			dbg("TxBeamforming not supported for %s\n", word);
			nvram_set(strcat_r(prefix, "txbf_bfr_cap", tmp), "0"); /* off = 0 */
			nvram_set(strcat_r(prefix, "txbf_bfe_cap", tmp), "0");
			nvram_set(strcat_r(prefix, "txbf", tmp), "0");
			nvram_set(strcat_r(prefix, "itxbf", tmp), "0");
			nvram_set(strcat_r(prefix, "txbf_imp", tmp), "0");
		}
		else {
			/* nothing to do right now! - use default nvram config or desired user wlan setup */
			dbg("TxBeamforming supported for %s - corerev: %s\n", word, buf);
			dbG("txbf_bfr_cap for %s = %s\n", word, nvram_safe_get(strcat_r(prefix, "txbf_bfr_cap", tmp)));
			dbG("txbf_bfe_cap for %s = %s\n", word, nvram_safe_get(strcat_r(prefix, "txbf_bfe_cap", tmp)));
		}

		if (nvram_match(strcat_r(prefix, "nband", tmp), "1") && /* only for wlX_nband == 1 for 5 GHz */
		    nvram_match(strcat_r(prefix, "vreqd", tmp), "1") &&
		    nvram_match(strcat_r(prefix, "nmode", tmp), "-1")) { /* only for mode AUTO == -1 */
		  
			dbG("set vhtmode 1 for %s\n", word);
			eval("wl", "-i", word, "vhtmode", "1");
		}
		else if (nvram_match(strcat_r(prefix, "nband", tmp), "2") && /* only for wlX_nband == 2 for 2,4 GHz */
			 nvram_match(strcat_r(prefix, "vreqd", tmp), "1") &&
			 nvram_match(strcat_r(prefix, "nmode", tmp), "-1")) { /* only for mode AUTO == -1 */
		  
		  	if (nvram_match(strcat_r(prefix, "turbo_qam", tmp), "1")) { /* check turbo qam on or off ? */
				dbG("set vht_features 3 for %s\n", word);
				eval("wl", "-i", word, "vht_features", "3");
		  	}
			else {
				dbG("set vht_features 0 for %s\n", word);
				eval("wl", "-i", word, "vht_features", "0");
			}

			dbG("set vhtmode 1 for %s\n", word);
			eval("wl", "-i", word, "vhtmode", "1");

		}
		else {
			dbG("set vhtmode 0 for %s\n", word);
			eval("wl", "-i", word, "vht_features", "0");
			eval("wl", "-i", word, "vhtmode", "0");
		}
		unit++;
	}
}
#endif /* TCONFIG_BCMWL6 */

static void set_lan_hostname(const char *wan_hostname)
{
	const char *s;
	FILE *f;

	nvram_set("lan_hostname", wan_hostname);
	if ((wan_hostname == NULL) || (*wan_hostname == 0)) {
		/* derive from et0 mac address */
		s = nvram_get("et0macaddr");
		if (s && strlen(s) >= 17) {
			char hostname[16];
			sprintf(hostname, "RT-%c%c%c%c%c%c%c%c%c%c%c%c",
				s[0], s[1], s[3], s[4], s[6], s[7],
				s[9], s[10], s[12], s[13], s[15], s[16]);

			if ((f = fopen("/proc/sys/kernel/hostname", "w"))) {
				fputs(hostname, f);
				fclose(f);
			}
			nvram_set("lan_hostname", hostname);
		}
	}

	if ((f = fopen("/etc/hosts", "w"))) {
		fprintf(f, "127.0.0.1  localhost\n");
		if ((s = nvram_get("lan_ipaddr")) && (*s))
			fprintf(f, "%s  %s %s-lan\n", s, nvram_safe_get("lan_hostname"), nvram_safe_get("lan_hostname"));
		if ((s = nvram_get("lan1_ipaddr")) && (*s) && (strcmp(s,"") != 0))
			fprintf(f, "%s  %s-lan1\n", s, nvram_safe_get("lan_hostname"));
		if ((s = nvram_get("lan2_ipaddr")) && (*s) && (strcmp(s,"") != 0))
			fprintf(f, "%s  %s-lan2\n", s, nvram_safe_get("lan_hostname"));
		if ((s = nvram_get("lan3_ipaddr")) && (*s) && (strcmp(s,"") != 0))
			fprintf(f, "%s  %s-lan3\n", s, nvram_safe_get("lan_hostname"));
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			fprintf(f, "::1  localhost\n");
			s = ipv6_router_address(NULL);
			if (*s) fprintf(f, "%s  %s\n", s, nvram_safe_get("lan_hostname"));
		}
#endif
		fclose(f);
	}
}

void set_host_domain_name(void)
{
	const char *s;

	s = nvram_safe_get("wan_hostname");
	sethostname(s, strlen(s));
	set_lan_hostname(s);

	s = nvram_get("wan_domain");
	if ((s == NULL) || (*s == 0)) s = nvram_safe_get("wan_get_domain");
	setdomainname(s, strlen(s));
}

static int wlconf(char *ifname, int unit, int subunit)
{
	int r = -1;
	char wl[24];

#ifdef TCONFIG_BCMWL6
	int phytype;
	char buf[8] = {0};
	char tmp[128] = {0};
#endif
	
#ifdef TCONFIG_BCMBSD
	char word[128], *next;
	char prefix[] = "wlXXXXXXX_";
	char prefix2[] = "wlXXXXXXX_";
	char tmp2[128];
	int wlif_count = 0;
	int i = 0;
#endif

	/* Check interface - fail for non-wl interfaces */
	if ((unit < 0) || wl_probe(ifname)) return r;

	dbG("wlconf: ifname %s unit %d subunit %d\n", ifname, unit, subunit);

#ifndef TCONFIG_BCMARM
	/* Tomato MIPS needs a little help here: wlconf() will not validate/restore all per-interface related variables right now; */
	snprintf(wl, sizeof(wl), "--wl%d", unit);
	eval("nvram", "validate", wl); /* sync wl_ and wlX ; (MIPS does not use nvram_tuple router_defaults; ARM branch does ... ) */
	memset(wl, 0, sizeof(wl)); /* reset */
#endif

#ifdef TCONFIG_BCMWL6
	/* set phytype */
	if ((subunit == -1) && !wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype))) {
		snprintf(wl, sizeof(wl), "wl%d_", unit);
		snprintf(buf, sizeof(buf), "%s", WLCONF_PHYTYPE2STR(phytype));
		nvram_set(strcat_r(wl, "phytype", tmp), buf);
		dbG("wlconf: %s = %s\n", tmp, buf);
		memset(wl, 0, sizeof(wl)); /* reset */
	}
#endif
	
#ifdef TCONFIG_BCMBSD
	if (subunit == -1) {
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
		if ((unit == 0) && nvram_get_int("smart_connect_x") == 1) { /* band steering enabled --> sync wireless settings to first module */

			foreach(word, nvram_safe_get("wl_ifnames"), next)
				wlif_count++;

			for (i = unit + 1; i < wlif_count; i++) {
				snprintf(prefix2, sizeof(prefix2), "wl%d_", i);
				nvram_set(strcat_r(prefix2, "ssid", tmp2), nvram_safe_get(strcat_r(prefix, "ssid", tmp)));
				nvram_set(strcat_r(prefix2, "key", tmp2), nvram_safe_get(strcat_r(prefix, "key", tmp)));
				nvram_set(strcat_r(prefix2, "key1", tmp2), nvram_safe_get(strcat_r(prefix, "key1", tmp)));
				nvram_set(strcat_r(prefix2, "key2", tmp2), nvram_safe_get(strcat_r(prefix, "key2", tmp)));
				nvram_set(strcat_r(prefix2, "key3", tmp2), nvram_safe_get(strcat_r(prefix, "key3", tmp)));
				nvram_set(strcat_r(prefix2, "key4", tmp2), nvram_safe_get(strcat_r(prefix, "key4", tmp)));
				nvram_set(strcat_r(prefix2, "crypto", tmp2), nvram_safe_get(strcat_r(prefix, "crypto", tmp)));
				nvram_set(strcat_r(prefix2, "wpa_psk", tmp2), nvram_safe_get(strcat_r(prefix, "wpa_psk", tmp)));
				nvram_set(strcat_r(prefix2, "radius_ipaddr", tmp2), nvram_safe_get(strcat_r(prefix, "radius_ipaddr", tmp)));
				nvram_set(strcat_r(prefix2, "radius_key", tmp2), nvram_safe_get(strcat_r(prefix, "radius_key", tmp)));
				nvram_set(strcat_r(prefix2, "radius_port", tmp2), nvram_safe_get(strcat_r(prefix, "radius_port", tmp)));
				nvram_set(strcat_r(prefix2, "closed", tmp2), nvram_safe_get(strcat_r(prefix, "closed", tmp)));
			}
		}
	}
#endif /* TCONFIG_BCMBSD */

	r = eval("wlconf", ifname, "up");
	dbG("wlconf %s = %d\n", ifname, r);
	if (r == 0) {
		if (unit >= 0 && subunit <= 0) {
			/* setup primary wl interface */
			nvram_set("rrules_radio", "-1");

			eval("wl", "-i", ifname, "antdiv", nvram_safe_get(wl_nvname("antdiv", unit, 0)));
			eval("wl", "-i", ifname, "txant", nvram_safe_get(wl_nvname("txant", unit, 0)));
			eval("wl", "-i", ifname, "txpwr1", "-o", "-m", nvram_get_int(wl_nvname("txpwr", unit, 0)) ? nvram_safe_get(wl_nvname("txpwr", unit, 0)) : "-1");
#ifdef TCONFIG_BCMWL6
			eval("wl", "-i", ifname, "interference", nvram_match(wl_nvname("phytype", unit, 0), "v") ? nvram_safe_get(wl_nvname("mitigation_ac", unit, 0)) : nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#else
			eval("wl", "-i", ifname, "interference", nvram_safe_get(wl_nvname("mitigation", unit, 0)));
#endif
		}

		if (wl_client(unit, subunit)) {
			if (nvram_match(wl_nvname("mode", unit, subunit), "wet")) {
				ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL);
			}
			if (nvram_get_int(wl_nvname("radio", unit, 0))) {
				snprintf(wl, sizeof(wl), "%d", unit);
				xstart("radio", "join", wl);
			}
		}
	}
	return r;
}

#ifdef TCONFIG_EMF
static void emf_mfdb_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *mgrp, *ifname;

	/* Add/Delete MFDB entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_entry"), next) {
		ifname = word;
		mgrp = strsep(&ifname, ":");

		if ((mgrp == NULL) || (ifname == NULL)) continue;

		/* Add/Delete MFDB entry using the group addr and interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "mfdb", lan_ifname, mgrp, ifname);
		}
	}
}

static void emf_uffp_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete UFFP entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_uffp_entry"), next) {
		ifname = word;

		if (ifname == NULL) continue;

		/* Add/Delete UFFP entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "uffp", lan_ifname, ifname);
		}
	}
}

static void emf_rtport_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete RTPORT entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_rtport_entry"), next) {
		ifname = word;

		if (ifname == NULL) continue;

		/* Add/Delete RTPORT entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "rtport", lan_ifname, ifname);
		}
	}
}

static void start_emf(char *lan_ifname)
{
	int ret = 0;
	char tmp[32] = {0};
#ifdef TCONFIG_BCMARM
	char tmp_path[64] = {0};

	if ((lan_ifname == NULL) ||
	    (strcmp(lan_ifname,"") == 0)) return;

	snprintf(tmp_path, sizeof(tmp_path), "/sys/class/net/%s/bridge/multicast_snooping", lan_ifname);

	/* make it possible to enable bridge multicast_snooping */
	if (nvram_get_int("br_mcast_snooping")) {
		f_write_string(tmp_path, "1", 0, 0);
	}
	else { /* DEFAULT: OFF - it can interfere with EMF */
		f_write_string(tmp_path, "0", 0, 0);
	}

	if (!nvram_get_int("emf_enable")) return;
#else /* for Tomato MIPS */
	if (lan_ifname == NULL || !nvram_get_int("emf_enable") || (strcmp(lan_ifname,"") == 0))
		return;
#endif /* TCONFIG_BCMARM */

	/* Start EMF */
	ret = eval("emf", "start", lan_ifname);

	/* Add the static MFDB entries */
	emf_mfdb_update(lan_ifname, NULL, 1);

	/* Add the UFFP entries */
	emf_uffp_update(lan_ifname, NULL, 1);

	/* Add the RTPORT entries */
	emf_rtport_update(lan_ifname, NULL, 1);

	if (ret) {
		logmsg(LOG_INFO, "starting EMF for %s failed ...\n", lan_ifname);
	}
	else {
		logmsg(LOG_INFO, "EMF for %s is started\n", lan_ifname);
		nvram_set(strcat_r(lan_ifname,"_emf_active", tmp), "1"); /* set active */
	}
}

static void stop_emf(char *lan_ifname)
{
	int ret = 0;
	char tmp[32] = {0};

	/* check if emf is active for lan_ifname */
	if (lan_ifname == NULL ||
	    !nvram_get_int(strcat_r(lan_ifname,"_emf_active", tmp))) return;

	/* Stop EMF for this LAN / brX */
	ret = eval("emf", "stop", lan_ifname);

	/* Remove bridge from igs */
	eval("igs", "del", "bridge", lan_ifname);
	eval("emf", "del", "bridge", lan_ifname);

	if (ret) {
		logmsg(LOG_INFO, "stopping EMF for %s failed ...\n", lan_ifname);
	}
	else {
		logmsg(LOG_INFO, "EMF for %s is stopped\n", lan_ifname);
		nvram_set(strcat_r(lan_ifname,"_emf_active", tmp), "0"); /* set NOT active */
	}
}
#endif

/* Set initial QoS mode for all et interfaces that are up. */
void set_et_qos_mode(void)
{
	int i, s, qos;
	struct ifreq ifr;
	struct ethtool_drvinfo info;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return;

	qos = (strcmp(nvram_safe_get("wl_wme"), "off") != 0);

	for (i = 1; i <= DEV_NUMIFS; i++) {
		ifr.ifr_ifindex = i;
		if (ioctl(s, SIOCGIFNAME, &ifr))
			continue;
		if (ioctl(s, SIOCGIFHWADDR, &ifr))
			continue;
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
			continue;
		if (ioctl(s, SIOCGIFFLAGS, &ifr))
			continue;
		if (!(ifr.ifr_flags & IFF_UP))
			continue;
		/* Set QoS for et & bcm57xx devices */
		memset(&info, 0, sizeof(info));
		info.cmd = ETHTOOL_GDRVINFO;
		ifr.ifr_data = (caddr_t)&info;
		if (ioctl(s, SIOCETHTOOL, &ifr) < 0)
			continue;
		if ((strncmp(info.driver, "et", 2) != 0) &&
		    (strncmp(info.driver, "bcm57", 5) != 0))
			continue;
		ifr.ifr_data = (caddr_t)&qos;
		ioctl(s, SIOCSETCQOS, &ifr);
	}

	close(s);
}

void unload_wl(void)
{
#ifdef TCONFIG_DHDAP
	modprobe_r("dhd");
#else
	/* do not unload the wifi driver by default, it can cause problems for some router */
	if (nvram_match("wl_unload_enable", "1")) {
		modprobe_r("wl");
	}
#endif
}

void load_wl(void)
{
#ifdef TCONFIG_DHDAP
	int i = 0, maxwl_eth = 0, maxunit = -1;
	int unit = -1;
	char ifname[16] = {0};
	char instance_base[128];

	/* Search for existing wl devices and the max unit number used */
	for (i = 1; i <= DEV_NUMIFS; i++) {
		snprintf(ifname, sizeof(ifname), "eth%d", i);
		if (!wl_probe(ifname)) {
			if (!wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
				maxwl_eth = i;
				maxunit = (unit > maxunit) ? unit : maxunit;
			}
		}
	}
	snprintf(instance_base, sizeof(instance_base), "instance_base=%d", maxunit + 1);
#ifdef TCONFIG_BCM7
	snprintf(instance_base, sizeof(instance_base), "%s", instance_base);
#endif
	eval("insmod", "dhd", instance_base);
#else
	modprobe("wl");
#endif
}

#ifdef CONFIG_BCMWL5
int disabled_wl(int idx, int unit, int subunit, void *param)
{
	char *ifname;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
	    !nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 1;

	return 0;
}
#endif

static int set_wlmac(int idx, int unit, int subunit, void *param)
{
	char *ifname;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
	    !nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

	if (strcmp(nvram_safe_get(wl_nvname("hwaddr", unit, subunit)), "") == 0) {
//		set_mac(ifname, wl_nvname("macaddr", unit, subunit),
		set_mac(ifname, wl_nvname("hwaddr", unit, subunit),  /* AB multiSSID */
		2 + unit + ((subunit > 0) ? ((unit + 1) * 0x10 + subunit) : 0));
	}
	else {
		set_mac(ifname, wl_nvname("hwaddr", unit, subunit), 0);
	}

	return 1;
}

void start_wl(void)
{
	restart_wl();
}

void restart_wl(void)
{
	char *lan_ifname, *lan_ifnames, *ifname, *p;
	int unit, subunit;
	int is_client = 0;
	int model;

	char tmp[32];
	char br;
	char prefix[16] = {0};

	int wlan_cnt = 0;
	int wlan_5g_cnt = 0;
	int wlan_52g_cnt = 0;
	char blink_wlan_ifname[32];
	char blink_wlan_5g_ifname[32];
	char blink_wlan_52g_ifname[32];

	/* get router model */
	model = get_model();

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);

		if (strncmp(lan_ifname, "br", 2) == 0) {
		
			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ifnames");

			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ') ++ifname;
					if (*ifname == 0) continue;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						char nv[40];
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
					}
					/* get the instance number of the wl i/f */
					else if (wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
						continue;

					is_client |= wl_client(unit, subunit) && nvram_get_int(wl_nvname("radio", unit, 0));

#ifdef CONFIG_BCMWL5
					memset(prefix, 0, sizeof(prefix));
					snprintf(prefix, sizeof(prefix), "wl%d_", unit);
					if (nvram_match(strcat_r(prefix, "radio", tmp), "0")) {
						eval("wlconf", ifname, "down");
					}
					else {
						eval("wlconf", ifname, "start"); /* start wl iface */
					}

					/* Enable WLAN LEDs if wireless interface is enabled */
					if (nvram_get_int(wl_nvname("radio", unit, 0))) {
						if ((wlan_cnt == 0) && (wlan_5g_cnt == 0) && (wlan_52g_cnt == 0) && (wlan_52g_cnt == 0)) {	/* kill all blink at first start up */
							killall("blink", SIGKILL);
							memset(blink_wlan_ifname, 0, sizeof(blink_wlan_ifname)); /* reset */
							memset(blink_wlan_5g_ifname, 0, sizeof(blink_wlan_5g_ifname));
							memset(blink_wlan_52g_ifname, 0, sizeof(blink_wlan_52g_ifname));
						}
						if (unit == 0) {
							led(LED_WLAN, LED_ON);	/* enable WLAN LED for 2.4 GHz */
							wlan_cnt++; /* count all wlan units / subunits */
							if (wlan_cnt < 2) strcpy(blink_wlan_ifname, ifname);
						}
						else if (unit == 1) {
							led(LED_5G, LED_ON);	/* enable WLAN LED for 5 GHz */
							wlan_5g_cnt++; /* count all 5g units / subunits */
							if (wlan_5g_cnt < 2) strcpy(blink_wlan_5g_ifname, ifname);
						}
						else if (unit == 2) {
							led(LED_52G, LED_ON);	/* enable WLAN LED for 2nd 5 GHz */
							wlan_52g_cnt++; /* count all 5g units / subunits */
							if (wlan_52g_cnt < 2) strcpy(blink_wlan_52g_ifname, ifname);
						}
					}
#endif	/* CONFIG_BCMWL5 */
				}
				free(lan_ifnames);
			}
		}
#ifdef CONFIG_BCMWL5
		else if (strcmp(lan_ifname, "")) {
			/* specific non-bridged lan iface */
			eval("wlconf", lan_ifname, "start");
		}
#endif	/* CONFIG_BCMWL5 */
	}

	killall("wldist", SIGTERM);
	eval("wldist");

	if (is_client)
		xstart("radio", "join");

	/* do some LED setup */
	if ((model == MODEL_WS880) ||
	    (model == MODEL_R6400) ||
	    (model == MODEL_R6400v2) ||
	    (model == MODEL_R6700v1) ||
	    (model == MODEL_R6700v3) ||
	    (model == MODEL_R7000) ||
	    (model == MODEL_XR300) ||
	    (model == MODEL_R8000)) {
		if (nvram_match("wl0_radio", "1") || nvram_match("wl1_radio", "1") || nvram_match("wl2_radio", "1"))
			led(LED_AOSS, LED_ON);
		else
			led(LED_AOSS, LED_OFF);
	}

	/* Finally: start blink (traffic "control" of LED) if only one unit (for each wlan) is enabled AND stealth mode is off */
	if (nvram_get_int("blink_wl") && nvram_match("stealth_mode", "0")) {
		if (wlan_cnt == 1) eval("blink", blink_wlan_ifname, "wlan", "10", "8192");
		if (wlan_5g_cnt == 1) eval("blink", blink_wlan_5g_ifname, "5g", "10", "8192");
		if (wlan_52g_cnt == 1) eval("blink", blink_wlan_52g_ifname, "52g", "10", "8192");
	}
}

void stop_lan_wl(void)
{
	char *p, *ifname;
	char *wl_ifnames;
	char *lan_ifname;
#ifdef CONFIG_BCMWL5
	int unit, subunit;
#endif

	eval("ebtables", "-F");

	char tmp[32];
	char br;

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifnames");
		if ((wl_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
			p = wl_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				while (*ifname == ' ') ++ifname;
				if (*ifname == 0) continue;
#ifdef CONFIG_BCMWL5
				if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
					if (get_ifname_unit(ifname, &unit, &subunit) < 0)
						continue;
				}
				else if (wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit))) {
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "del", "iface", lan_ifname, ifname);
#endif
					continue;
				}

				eval("wlconf", ifname, "down");
#endif
				ifconfig(ifname, 0, NULL, NULL);
				eval("brctl", "delif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
				if (nvram_get_int("emf_enable"))
					eval("emf", "del", "iface", lan_ifname, ifname);
#endif

			}
			free(wl_ifnames);
		}
#ifdef TCONFIG_EMF
	stop_emf(lan_ifname);
#endif
	}

}

void start_lan_wl(void)
{
	char *lan_ifname;

	char *wl_ifnames, *ifname, *p;
	uint32 ip;
	int unit, subunit, sta;

	char tmp[32];
	char br;

#ifdef TCONFIG_DHDAP
	int is_dhd;
#endif /* TCONFIG_DHDAP */

#ifdef CONFIG_BCMWL5
	foreach_wif(0, NULL, set_wlmac);
#else
	foreach_wif(1, NULL, set_wlmac);
#endif

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);

		if (strncmp(lan_ifname, "br", 2) == 0) {

#ifdef TCONFIG_EMF
			if (nvram_get_int("emf_enable")) {
				eval("emf", "add", "bridge", lan_ifname);
				eval("igs", "add", "bridge", lan_ifname);
			}
#endif
			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ipaddr");
			inet_aton(nvram_safe_get(tmp), (struct in_addr *)&ip);

			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ifnames");

			sta = 0;

			if ((wl_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = wl_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ') ++ifname;
					if (*ifname == 0) continue;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
#ifdef CONFIG_BCMWL5
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
#endif
						char nv[40];
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
#ifdef CONFIG_BCMWL5
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
						set_wlmac(0, unit, subunit, NULL);
					}
					else
						wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));
#endif

					/* bring up interface */
					if (ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL) != 0) continue;

#ifdef CONFIG_BCMWL5
					if (wlconf(ifname, unit, subunit) == 0) {
						const char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

						if (strcmp(mode, "wet") == 0) {
							/* Enable host DHCP relay */
							if (nvram_get_int("dhcp_relay")) { /* only set "wet_host_ipv4" (again), "wet_host_mac" already set at start_lan() */
#if !defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP) /* only for ARM dual-core SDK6 starting with ~ AiMesh 2.0 support / ~ October 2020 */
								if (subunit > 0) { /* only for enabled subunits */
									wet_host_t wh;

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, &ip, sizeof(ip)); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_ipv4", &wh, sizeof(wet_host_t));
								}
#else
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if(is_dhd) {
									dhd_iovar_setint(ifname, "wet_host_ipv4", ip);
								}
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_setint(ifname, "wet_host_ipv4", ip);
#endif /* !defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP) */
							}
						}

						sta |= (strcmp(mode, "sta") == 0);
						if ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0)) continue;
					}
#endif
					eval("brctl", "addif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "add", "iface", lan_ifname, ifname);
#endif
				}
				free(wl_ifnames);
			}
		}
#ifdef TCONFIG_EMF
		start_emf(lan_ifname);
#endif
	}
}

void stop_wireless(void) {
#ifdef CONFIG_BCMWL5
	stop_nas();
#endif
	stop_lan_wl();

	//unload_wl();
}

void start_wireless(void) {
	//load_wl();

	start_lan_wl();

#ifdef CONFIG_BCMWL5
	start_nas();
#endif
	restart_wl();
}

void restart_wireless(void) {

	stop_wireless();

	start_wireless();

}

#ifdef TCONFIG_IPV6
void enable_ipv6(int enable)
{
	DIR *dir;
	struct dirent *dirent;
	char s[128];

	if ((dir = opendir("/proc/sys/net/ipv6/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			/* Do not enable IPv6 on 'all', 'eth0', 'eth1', 'eth2' (ethX);
			 * IPv6 will live on the bridged instances --> This simplifies the routing table a little bit;
			 * 'default' is enabled so that new interfaces (brX, vlanX, ...) will get IPv6
			 */
			if ((strncmp("eth", dirent->d_name, 3) == 0) || (strncmp("all", dirent->d_name, 3) == 0)) {
				/* do nothing */
			}
			else {
				snprintf(s, sizeof(s), "ipv6/conf/%s/disable_ipv6", dirent->d_name);
				f_write_procsysnet(s, enable ? "0" : "1");
			}
		}
		closedir(dir);
	}
}

void accept_ra(const char *ifname)
{
	char s[128];

	/* possible values for accept_ra:
	   0 Do not accept Router Advertisements
	   1 Accept Router Advertisements if forwarding is disabled
	   2 Overrule forwarding behaviour. Accept Router Advertisements even if forwarding is enabled
	*/
	snprintf(s, sizeof(s), "ipv6/conf/%s/accept_ra", ifname);
	f_write_procsysnet(s, "2");
}

void accept_ra_reset(const char *ifname)
{
	char s[128];

	/* set accept_ra (back) to 1 (default) */
	snprintf(s, sizeof(s), "ipv6/conf/%s/accept_ra", ifname);
	f_write_procsysnet(s, "1");
}

void ipv6_forward(const char *ifname, int enable)
{
	char s[128];

	/* possible values for forwarding:
	   0 Forwarding disabled
	   1 Forwarding enabled
	*/
	snprintf(s, sizeof(s), "ipv6/conf/%s/forwarding", ifname);
	f_write_procsysnet(s, enable ? "1" : "0");
}

void ndp_proxy(const char *ifname, int enable)
{
	char s[128];

	snprintf(s, sizeof(s), "ipv6/conf/%s/proxy_ndp", ifname);
	f_write_procsysnet(s, enable ? "1" : "0");

}
#endif /* TCONFIG_IPV6 */

void start_lan(void)
{
	logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, __LINE__);

	char *lan_ifname;
	struct ifreq ifr;
	char *lan_ifnames, *ifname, *p;
	int sfd;
	uint32 ip;
	int unit, subunit, sta;
	int hwaddrset;
	char eabuf[32];
	char tmp[32];
	char tmp2[32];
	char br;
	int vlan0tag;
	int vid;
	int vid_map;
	char *iftmp;
	char nv[64];

#ifdef TCONFIG_DHDAP
	int is_dhd;
#endif /* TCONFIG_DHDAP */

#ifndef TCONFIG_DHDAP /* load driver at init.c for sdk7 */
	load_wl(); /* lets go! */
#endif

#ifdef TCONFIG_BCMWL6
	wlconf_pre(); /* prepare a few wifi things */
#endif

#ifdef CONFIG_BCMWL5
	foreach_wif(0, NULL, set_wlmac);
#else
	foreach_wif(1, NULL, set_wlmac);
#endif

#ifdef TCONFIG_IPV6
	enable_ipv6(ipv6_enabled());  /* tell Kernel to disable/enable IPv6 for most interfaces */
#endif
	vlan0tag = nvram_get_int("vlan0tag");

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return;

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = strdup(nvram_safe_get(tmp));

		if (strncmp(lan_ifname, "br", 2) == 0) {
			logmsg(LOG_DEBUG, "*** %s: setting up the bridge %s", __FUNCTION__, lan_ifname);

			eval("brctl", "addbr", lan_ifname);
			eval("brctl", "setfd", lan_ifname, "0");
			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_stp");
			eval("brctl", "stp", lan_ifname, nvram_safe_get(tmp));

#ifdef TCONFIG_EMF
			if (nvram_get_int("emf_enable")) {
				eval("emf", "add", "bridge", lan_ifname);
				eval("igs", "add", "bridge", lan_ifname);
			}
#endif

			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ipaddr");
			inet_aton(nvram_safe_get(tmp), (struct in_addr *)&ip);

			hwaddrset = 0;
			sta = 0;

			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ifnames");
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((iftmp = strsep(&p, " ")) != NULL) {
					while (*iftmp == ' ') ++iftmp;
					if (*iftmp == 0) continue;
					ifname = iftmp;

					unit = -1; subunit = -1;

					/* ignore disabled wl vifs */
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {

						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
#ifdef CONFIG_BCMWL5
						set_wlmac(0, unit, subunit, NULL);
#endif
					}
					else
						wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));

					/* vlan ID mapping */
					if (strncmp(ifname, "vlan", 4) == 0) {
						if (sscanf(ifname, "vlan%d", &vid) == 1) {
							snprintf(tmp, sizeof(tmp), "vlan%dvid", vid);
							vid_map = nvram_get_int(tmp);
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | vid;
							snprintf(tmp, sizeof(tmp), "vlan%d", vid_map);
							ifname = tmp;
						}
					}

					/* bring up interface */
					if (ifconfig(ifname, IFUP | IFF_ALLMULTI, NULL, NULL) != 0) continue;

					/* set the logical bridge address to that of the first interface */
					strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
					if ((!hwaddrset) ||
						(ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0 &&
						memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN) == 0)) {
						strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
						if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
							strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
							ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
							logmsg(LOG_DEBUG, "*** %s: setting MAC of %s bridge to %s", __FUNCTION__, ifr.ifr_name, ether_etoa((const unsigned char *)ifr.ifr_hwaddr.sa_data, eabuf));
							ioctl(sfd, SIOCSIFHWADDR, &ifr);
							hwaddrset = 1;
						}
					}

					if (wlconf(ifname, unit, subunit) == 0) {
						const char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

						if (strcmp(mode, "wet") == 0) {
							/* Enable host DHCP relay */
							if (nvram_get_int("dhcp_relay")) { /* set mac and ip */
#if !defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP) /* only for ARM dual-core SDK6 starting with ~ AiMesh 2.0 support / ~ October 2020 */
								if (subunit > 0) { /* only for enabled subunits */
									wet_host_t wh;

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_mac", &wh, ETHER_ADDR_LEN); /* set mac */

									memset(&wh, 0, sizeof(wet_host_t));
									wh.bssidx = subunit;
									memcpy(&wh.buf, &ip, sizeof(ip)); /* struct for ip or mac */

									wl_iovar_set(ifname, "wet_host_ipv4", &wh, sizeof(wet_host_t)); /* set ip */
								}
#else
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if(is_dhd) {
									char macbuf[sizeof("wet_host_mac") + 1 + ETHER_ADDR_LEN];
									dhd_iovar_setbuf(ifname, "wet_host_mac", ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN , macbuf, sizeof(macbuf)); /* set mac */
								}
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_set(ifname, "wet_host_mac", ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN); /* set mac */
#ifdef TCONFIG_DHDAP
								is_dhd = !dhd_probe(ifname);
								if(is_dhd) {
									dhd_iovar_setint(ifname, "wet_host_ipv4", ip); /* set ip */
								}
								else
#endif /* TCONFIG_DHDAP */
									wl_iovar_setint(ifname, "wet_host_ipv4", ip); /* set ip */
#endif /* !defined(TCONFIG_BCM7) && defined(TCONFIG_BCMSMP) */
							}
						}

						sta |= (strcmp(mode, "sta") == 0);
						if ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0)) continue;
					}
					eval("brctl", "addif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "add", "iface", lan_ifname, ifname);
#endif
				}
				free(lan_ifnames);
			}
		}
		/* --- this shouldn't happen --- */
		else if (*lan_ifname) {
			ifconfig(lan_ifname, IFUP, NULL, NULL);
			wlconf(lan_ifname, -1, -1);
		}
		else {
			close(sfd);
			free(lan_ifname);
#ifdef TCONFIG_IPV6
			start_ipv6(); /* all work done at function start_lan(), lets call start_ipv6() finally (only once!) */
#endif
			return;
		}

		/* Get current LAN hardware address */
		strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_hwaddr");
		if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
				nvram_set(tmp, ether_etoa((const unsigned char *)ifr.ifr_hwaddr.sa_data, eabuf));
		}

		close(sfd); /* close file descriptor */

		/* Set initial QoS mode for LAN ports */
#ifdef CONFIG_BCMWL5
		set_et_qos_mode();
#endif

		/* bring up and configure LAN interface */
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ipaddr");
		strcpy(tmp2,"lan");
		strcat(tmp2,bridge);
		strcat(tmp2, "_netmask");
		ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, nvram_safe_get(tmp), nvram_safe_get(tmp2));

		config_loopback();
		do_static_routes(1);

		if(br == 0)
			set_lan_hostname(nvram_safe_get("wan_hostname"));

		if ((get_wan_proto() == WP_DISABLED) && (br == 0)) {
			char *gateway = nvram_safe_get("lan_gateway") ;
			if ((*gateway) && (strcmp(gateway, "0.0.0.0") != 0)) {
				int tries = 5;
				while ((route_add(lan_ifname, 0, "0.0.0.0", gateway, "0.0.0.0") != 0) && (tries-- > 0)) sleep(1);
				logmsg(LOG_DEBUG, "*** %s: add gateway=%s tries=%d", __FUNCTION__, gateway, tries);
			}
		}

#ifdef TCONFIG_EMF
		start_emf(lan_ifname);
#endif

		free(lan_ifname);

	} /* for-loop brX */

	logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, __LINE__);
}

void stop_lan(void)
{
	logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, __LINE__);

	char *lan_ifname;
	char *lan_ifnames, *p, *ifname;
	char tmp[32];
	char br;
	int vlan0tag, vid, vid_map;
	char *iftmp;

	vlan0tag = nvram_get_int("vlan0tag");

	ifconfig("lo", 0, NULL, NULL); /* Bring down loopback interface */

#ifdef TCONFIG_IPV6
	stop_ipv6(); /* stop IPv6 first! */
#endif

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);
		ifconfig(lan_ifname, 0, NULL, NULL);

		if (strncmp(lan_ifname, "br", 2) == 0) {

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifnames");
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((iftmp = strsep(&p, " ")) != NULL) {
					while (*iftmp == ' ') ++iftmp;
					if (*iftmp == 0) continue;
					ifname = iftmp;
					/* vlan ID mapping */
					if (strncmp(ifname, "vlan", 4) == 0) {
						if (sscanf(ifname, "vlan%d", &vid) == 1) {
							snprintf(tmp, sizeof(tmp), "vlan%dvid", vid);
							vid_map = nvram_get_int(tmp);
							if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | vid;
							snprintf(tmp, sizeof(tmp), "vlan%d", vid_map);
							ifname = tmp;
						}
					}
					eval("wlconf", ifname, "down");
					ifconfig(ifname, 0, NULL, NULL);
					eval("brctl", "delif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "del", "iface", lan_ifname, ifname);
#endif
				}
				free(lan_ifnames);
			}
#ifdef TCONFIG_EMF
			stop_emf(lan_ifname);
#endif
			eval("brctl", "delbr", lan_ifname);
		}
		else if (*lan_ifname) {
			eval("wlconf", lan_ifname, "down");
		}
	}
	logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, __LINE__);

#ifndef TCONFIG_DHDAP /* do not unload driver for sdk7 */
	unload_wl(); /* stop! */
#endif
}

static int is_sta(int idx, int unit, int subunit, void *param)
{
	return (nvram_match(wl_nvname("mode", unit, subunit), "sta") && (nvram_match(wl_nvname("bss_enabled", unit, subunit), "1")));
}

void do_static_routes(int add)
{
	char *buf;
	char *p, *q;
	char *dest, *mask, *gateway, *metric, *ifname;
	int r;

	if ((buf = strdup(nvram_safe_get(add ? "routes_static" : "routes_static_saved"))) == NULL) return;
	if (add) nvram_set("routes_static_saved", buf);
		else nvram_unset("routes_static_saved");
	p = buf;
	while ((q = strsep(&p, ">")) != NULL) {
		if (vstrsep(q, "<", &dest, &gateway, &mask, &metric, &ifname) < 5)
			continue;

		ifname = nvram_safe_get(((strcmp(ifname,"LAN")==0) ? "lan_ifname" :
					((strcmp(ifname,"LAN1")==0) ? "lan1_ifname" :
					((strcmp(ifname,"LAN2")==0) ? "lan2_ifname" :
					((strcmp(ifname,"LAN3")==0) ? "lan3_ifname" :
					((strcmp(ifname,"WAN2")==0) ? "wan2_iface" :
					((strcmp(ifname,"WAN3")==0) ? "wan3_iface" :
					((strcmp(ifname,"WAN4")==0) ? "wan4_iface" :
					((strcmp(ifname,"MAN2")==0) ? "wan2_ifname" :
					((strcmp(ifname,"MAN3")==0) ? "wan3_ifname" :
					((strcmp(ifname,"MAN4")==0) ? "wan4_ifname" :
					((strcmp(ifname,"WAN")==0) ? "wan_iface" : "wan_ifname"))))))))))));
		logmsg(LOG_WARNING, "Static route, ifname=%s, metric=%s, dest=%s, gateway=%s, mask=%s", ifname, metric, dest, gateway, mask);

		if (add) {
			for (r = 3; r >= 0; --r) {
				if (route_add(ifname, atoi(metric), dest, gateway, mask) == 0) break;
				sleep(1);
			}
		}
		else {
			route_del(ifname, atoi(metric), dest, gateway, mask);
		}
	}
	free(buf);

	char *wan_modem_ipaddr;
	if ( (nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "dhcp") || nvram_match("wan_proto", "static") )
	    && (wan_modem_ipaddr = nvram_safe_get("wan_modem_ipaddr")) && *wan_modem_ipaddr && !nvram_match("wan_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta)) ) {
		char ip[16];
		char *end = rindex(wan_modem_ipaddr,'.')+1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan_ifname");

		sprintf(ip, "%.*s%hhu", end-wan_modem_ipaddr, wan_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add":"del", ip, "peer", wan_modem_ipaddr, "dev", iface);
	}

	char *wan2_modem_ipaddr;
	if ( (nvram_match("wan2_proto", "pppoe") || nvram_match("wan2_proto", "dhcp") || nvram_match("wan2_proto", "static") )
	    && (wan2_modem_ipaddr = nvram_safe_get("wan2_modem_ipaddr")) && *wan2_modem_ipaddr && !nvram_match("wan2_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta)) ) {
		char ip[16];
		char *end = rindex(wan2_modem_ipaddr,'.')+1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan2_ifname");

		sprintf(ip, "%.*s%hhu", end-wan2_modem_ipaddr, wan2_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add":"del", ip, "peer", wan2_modem_ipaddr, "dev", iface);
	}

#ifdef TCONFIG_MULTIWAN
	char *wan3_modem_ipaddr;
	if ( (nvram_match("wan3_proto", "pppoe") || nvram_match("wan3_proto", "dhcp") || nvram_match("wan3_proto", "static") )
	    && (wan3_modem_ipaddr = nvram_safe_get("wan3_modem_ipaddr")) && *wan3_modem_ipaddr && !nvram_match("wan3_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta)) ) {
		char ip[16];
		char *end = rindex(wan3_modem_ipaddr,'.')+1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan3_ifname");

		sprintf(ip, "%.*s%hhu", end-wan3_modem_ipaddr, wan3_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add":"del", ip, "peer", wan3_modem_ipaddr, "dev", iface);
	}

	char *wan4_modem_ipaddr;
	if ( (nvram_match("wan4_proto", "pppoe") || nvram_match("wan4_proto", "dhcp") || nvram_match("wan4_proto", "static") )
	    && (wan4_modem_ipaddr = nvram_safe_get("wan4_modem_ipaddr")) && *wan4_modem_ipaddr && !nvram_match("wan4_modem_ipaddr","0.0.0.0") 
	    && (!foreach_wif(1, NULL, is_sta)) ) {
		char ip[16];
		char *end = rindex(wan4_modem_ipaddr,'.')+1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan4_ifname");

		sprintf(ip, "%.*s%hhu", end-wan4_modem_ipaddr, wan4_modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add":"del", ip, "peer", wan4_modem_ipaddr, "dev", iface);
	}
#endif
}

void hotplug_net(void)
{
	char *interface, *action;
	char *lan_ifname;

	if (((interface = getenv("INTERFACE")) == NULL) || ((action = getenv("ACTION")) == NULL)) return;

	logmsg(LOG_DEBUG, "*** %s: hotplug net INTERFACE=%s ACTION=%s", __FUNCTION__, interface, action);

	if ((strncmp(interface, "wds", 3) == 0) &&
	    (strcmp(action, "register") == 0 || strcmp(action, "add") == 0)) {
		ifconfig(interface, IFUP, NULL, NULL);
		lan_ifname = nvram_safe_get("lan_ifname");
#ifdef TCONFIG_EMF
		if (nvram_get_int("emf_enable")) {
			eval("emf", "add", "iface", lan_ifname, interface);
			emf_mfdb_update(lan_ifname, interface, 1);
			emf_uffp_update(lan_ifname, interface, 1);
			emf_rtport_update(lan_ifname, interface, 1);
		}
#endif
		if (strncmp(lan_ifname, "br", 2) == 0) {
			eval("brctl", "addif", lan_ifname, interface);
			notify_nas(interface);
		}
	}
}


static int is_same_addr(struct ether_addr *addr1, struct ether_addr *addr2)
{
	int i;
	for (i = 0; i < 6; i++) {
		if (addr1->octet[i] != addr2->octet[i])
			return 0;
	}
	return 1;
}

#define WL_MAX_ASSOC	128
static int check_wl_client(char *ifname, int unit, int subunit)
{
	struct ether_addr bssid;
	wl_bss_info_t *bi;
	char buf[WLC_IOCTL_MAXLEN];
	struct maclist *mlist;
	unsigned int i;
	int mlsize;
	int associated, authorized;

	*(uint32 *)buf = WLC_IOCTL_MAXLEN;
	if (wl_ioctl(ifname, WLC_GET_BSSID, &bssid, ETHER_ADDR_LEN) < 0 ||
	    wl_ioctl(ifname, WLC_GET_BSS_INFO, buf, WLC_IOCTL_MAXLEN) < 0)
		return 0;

	bi = (wl_bss_info_t *)(buf + 4);
	if ((bi->SSID_len == 0) ||
	    (bi->BSSID.octet[0] + bi->BSSID.octet[1] + bi->BSSID.octet[2] +
	     bi->BSSID.octet[3] + bi->BSSID.octet[4] + bi->BSSID.octet[5] == 0))
		return 0;

	associated = 0;
	authorized = strstr(nvram_safe_get(wl_nvname("akm", unit, subunit)), "psk") == 0;

	mlsize = sizeof(struct maclist) + (WL_MAX_ASSOC * sizeof(struct ether_addr));
	if ((mlist = malloc(mlsize)) != NULL) {
		mlist->count = WL_MAX_ASSOC;
		if (wl_ioctl(ifname, WLC_GET_ASSOCLIST, mlist, mlsize) == 0) {
			for (i = 0; i < mlist->count; ++i) {
				if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
					associated = 1;
					break;
				}
			}
		}

		if (associated && !authorized) {
			memset(mlist, 0, mlsize);
			mlist->count = WL_MAX_ASSOC;
			strcpy((char*)mlist, "autho_sta_list");
			if (wl_ioctl(ifname, WLC_GET_VAR, mlist, mlsize) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
						authorized = 1;
						break;
					}
				}
			}
		}
		free(mlist);
	}

	return (associated && authorized);
}

#define STACHECK_CONNECT	30
#define STACHECK_DISCONNECT	5

static int radio_join(int idx, int unit, int subunit, void *param)
{
	int i;
	char s[32], f[64];
	char *ifname;

	int *unit_filter = param;
	if (*unit_filter >= 0 && *unit_filter != unit) return 0;

	if (!nvram_get_int(wl_nvname("radio", unit, 0)) || !wl_client(unit, subunit)) return 0;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	/* skip disabled wl vifs */
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
		!nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

	sprintf(f, "/var/run/radio.%d.%d.pid", unit, subunit < 0 ? 0 : subunit);
	if (f_read_string(f, s, sizeof(s)) > 0) {
		if ((i = atoi(s)) > 1) {
			kill(i, SIGTERM);
			sleep(1);
		}
	}

	if (fork() == 0) {
		sprintf(s, "%d", getpid());
		f_write(f, s, sizeof(s), 0, 0644);

		int stacheck_connect = nvram_get_int("sta_chkint");
		if (stacheck_connect <= 0)
			stacheck_connect = STACHECK_CONNECT;
		int stacheck;

		while (get_radio(unit) && wl_client(unit, subunit)) {

			if (check_wl_client(ifname, unit, subunit)) {
				stacheck = stacheck_connect;
			}
			else {
				eval("wl", "-i", ifname, "disassoc");
#ifdef CONFIG_BCMWL5
				char *amode, *sec = nvram_safe_get(wl_nvname("akm", unit, subunit));

				if (strstr(sec, "psk2")) amode = "wpa2psk";
				else if (strstr(sec, "psk")) amode = "wpapsk";
				else if (strstr(sec, "wpa2")) amode = "wpa2";
				else if (strstr(sec, "wpa")) amode = "wpa";
				else if (nvram_get_int(wl_nvname("auth", unit, subunit))) amode = "shared";
				else amode = "open";

				eval("wl", "-i", ifname, "join", nvram_safe_get(wl_nvname("ssid", unit, subunit)),
					"imode", "bss", "amode", amode);
#else
				eval("wl", "-i", ifname, "join", nvram_safe_get(wl_nvname("ssid", unit, subunit)));
#endif
				stacheck = STACHECK_DISCONNECT;
			}
			sleep(stacheck);
		}
		unlink(f);
	}

	return 1;
}

enum {
	RADIO_OFF = 0,
	RADIO_ON = 1,
	RADIO_TOGGLE = 2
};

static int radio_toggle(int idx, int unit, int subunit, void *param)
{
	if (!nvram_get_int(wl_nvname("radio", unit, 0))) return 0;

	int *op = param;

	if (*op == RADIO_TOGGLE) {
		*op = get_radio(unit) ? RADIO_OFF : RADIO_ON;
	}

	set_radio(*op, unit);
	return *op;
}

int radio_main(int argc, char *argv[])
{
	int op = RADIO_OFF;
	int unit;

	if (argc < 2) {
HELP:
		usage_exit(argv[0], "on|off|toggle|join [N]\n");
	}
	unit = (argc == 3) ? atoi(argv[2]) : -1;

	if (strcmp(argv[1], "toggle") == 0)
		op = RADIO_TOGGLE;
	else if (strcmp(argv[1], "off") == 0)
		op = RADIO_OFF;
	else if (strcmp(argv[1], "on") == 0)
		op = RADIO_ON;
	else if (strcmp(argv[1], "join") == 0)
		goto JOIN;
	else
		goto HELP;

	if (unit >= 0)
		op = radio_toggle(0, unit, 0, &op);
	else
		op = foreach_wif(0, &op, radio_toggle);
		
	if (!op) {
		led(LED_DIAG, LED_OFF);
		return 0;
	}
JOIN:
	foreach_wif(1, &unit, radio_join);
	return 0;
}

/*
int wdist_main(int argc, char *argv[])
{
	int n;
	rw_reg_t r;
	int v;

	if (argc != 2) {
		r.byteoff = 0x684;
		r.size = 2;
		if (wl_ioctl(nvram_safe_get("wl_ifname"), 101, &r, sizeof(r)) == 0) {
			v = r.val - 510;
			if (v <= 9) v = 0;
				else v = (v - (9 + 1)) * 150;
			printf("Current: %d-%dm (0x%02x)\n\n", v + (v ? 1 : 0), v + 150, r.val);
		}
		usage_exit(argv[0], "<meters>");
	}
	if ((n = atoi(argv[1])) <= 0) setup_wldistance();
		else set_wldistance(n);
	return 0;
}
*/

static int get_wldist(int idx, int unit, int subunit, void *param)
{
	int n;

	char *p = nvram_safe_get(wl_nvname("distance", unit, 0));
	if ((*p == 0) || ((n = atoi(p)) < 0)) return 0;

	return (9 + (n / 150) + ((n % 150) ? 1 : 0));
}

static int wldist(int idx, int unit, int subunit, void *param)
{
	rw_reg_t r;
	uint32 s;
	char *p;
	int n;

	n = get_wldist(idx, unit, subunit, param);
	if (n > 0) {
		s = 0x10 | (n << 16);
		p = nvram_safe_get(wl_nvname("ifname", unit, 0));
		wl_ioctl(p, 197, &s, sizeof(s));

		r.byteoff = 0x684;
		r.val = n + 510;
		r.size = 2;
		wl_ioctl(p, 102, &r, sizeof(r));
	}
	return 0;
}

// ref: wificonf.c
int wldist_main(int argc, char *argv[])
{
	if (fork() == 0) {
		if (foreach_wif(0, NULL, get_wldist) == 0) return 0;

		while (1) {
			foreach_wif(0, NULL, wldist);
			sleep(2);
		}
	}

	return 0;
}
