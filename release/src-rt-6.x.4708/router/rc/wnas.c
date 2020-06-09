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

#include "rc.h"

#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <bcmutils.h>
#include <wlutils.h>


//	ref: http://wiki.openwrt.org/OpenWrtDocs/nas

//	#define DEBUG_TIMING

void notify_nas(const char *ifname);

static int security_on(int idx, int unit, int subunit, void *param)
{
	return nvram_get_int(wl_nvname("radio", unit, 0)) && (!nvram_match(wl_nvname("security_mode", unit, subunit), "disabled"));
}

static int is_wds(int idx, int unit, int subunit, void *param)
{
	return nvram_get_int(wl_nvname("radio", unit, 0)) && nvram_get_int(wl_nvname("wds_enable", unit, subunit));
}

#ifndef CONFIG_BCMWL5
static int is_sta(int idx, int unit, int subunit, void *param)
{
	return nvram_match(wl_nvname("mode", unit, subunit), "sta");
}
#endif

int wds_enable(void)
{
	return foreach_wif(1, NULL, is_wds);
}

int wl_security_on(void) {
	return foreach_wif(1, NULL, security_on);
}

#ifndef CONFIG_BCMWL5
static int nas_starter(int idx, int unit, int subunit, void *param) {
	char unit_str[] = "000000";
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ifnames[] = "lanXX_ifnames";
	char br;
	if(nvram_get_int(wl_nvname("bss_enabled", unit, subunit))) {
		if (nvram_match(wl_nvname("mode", unit, subunit), "ap")) {
			if (subunit > 0)
				snprintf(unit_str, sizeof(unit_str), "%d.%d", unit, subunit);
			else
				snprintf(unit_str, sizeof(unit_str), "%d", unit);

			for(br=3 ; br>=0 ; br--) {
				char bridge[2] = "0";
				if (br!=0)
					bridge[0]+=br;
				else
					strcpy(bridge, "");

				snprintf(lanN_ifname, sizeof(lanN_ifname), "lan%s_ifname", bridge);
				snprintf(lanN_ifnames, sizeof(lanN_ifnames), "lan%s_ifnames", bridge);

				if(strstr(nvram_safe_get(lanN_ifnames),nvram_safe_get(wl_nvname("ifname", unit, subunit))) != NULL) {
					xstart("/usr/sbin/nas.sh", unit_str, nvram_safe_get(lanN_ifname));
					sleep(3);
				}
			}
		}
		if(is_sta(idx, unit, subunit, NULL)) {
			xstart("nas", "/etc/nas.wan.conf", "/var/run/nas.wan.pid", "wan");
			sleep(1);
		}
	}
	return 0;
}
#endif

void start_nas(void)
{
	if (!foreach_wif(1, NULL, security_on)) {
		return;
	}

#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: uptime=%ld\n", __FUNCTION__, si.uptime);
#else
	_dprintf("%s\n", __FUNCTION__);
#endif

	stop_nas();

#ifdef CONFIG_BCMWL5
	setenv("UDP_BIND_IP", "127.0.0.1", 1);
	eval("eapd");
	unsetenv("UDP_BIND_IP");
	eval("nas");
#else
	mode_t m;

	m = umask(0077);

	if(nvram_get_int("nas_alternate")) {
		foreach_wif(1, NULL, nas_starter);
	} else {
		if(strstr(nvram_safe_get("lan_ifnames"),nvram_safe_get("wl0_ifname")) != NULL)
			xstart("nas", "/etc/nas.conf", "/var/run/nas.pid", "lan");
		if(strstr(nvram_safe_get("lan1_ifnames"),nvram_safe_get("wl0_ifname")) != NULL)
			xstart("nas", "/etc/nas.conf", "/var/run/nas.pid", "lan1");
		if(strstr(nvram_safe_get("lan2_ifnames"),nvram_safe_get("wl0_ifname")) != NULL)
			xstart("nas", "/etc/nas.conf", "/var/run/nas.pid", "lan2");
		if(strstr(nvram_safe_get("lan3_ifnames"),nvram_safe_get("wl0_ifname")) != NULL)
			xstart("nas", "/etc/nas.conf", "/var/run/nas.pid", "lan3");

		if (foreach_wif(1, NULL, is_sta))
			xstart("nas", "/etc/nas.wan.conf", "/var/run/nas.wan.pid", "wan");
	}
	if (foreach_wif(1, NULL, is_sta))
		xstart("nas", "/etc/nas.wan.conf", "/var/run/nas.wan.pid", "wan");
	umask(m);
#endif /* CONFIG_BCMWL5 */

	if (wds_enable()) {
		// notify NAS of all wds up ifaces upon startup
		FILE *fd;
		char *ifname, buf[256];

		if ((fd = fopen("/proc/net/dev", "r")) != NULL) {
			fgets(buf, sizeof(buf) - 1, fd);	// header lines
			fgets(buf, sizeof(buf) - 1, fd);
			while (fgets(buf, sizeof(buf) - 1, fd)) {
				if ((ifname = strchr(buf, ':')) == NULL) continue;
				*ifname = 0;
				if ((ifname = strrchr(buf, ' ')) == NULL) ifname = buf;
				else ++ifname;
				if (strstr(ifname, "wds")) {
					notify_nas(ifname);
				}
			}
			fclose(fd);
		}
	}
}

void stop_nas(void)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: uptime=%ld\n", __FUNCTION__, si.uptime);
#else
	_dprintf("%s\n", __FUNCTION__);
#endif

	killall_tk("nas");
#ifdef CONFIG_BCMWL5
	killall_tk("eapd");
#endif /* CONFIG_BCMWL5 */
}

void notify_nas(const char *ifname)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: ifname=%s uptime=%ld\n", __FUNCTION__, ifname, si.uptime);
#else
	_dprintf("%s: ifname=%s\n", __FUNCTION__, ifname);
#endif

#ifdef CONFIG_BCMWL5

	/* Inform driver to send up new WDS link event */
	if (wl_iovar_setint((char *)ifname, "wds_enable", 1)) {
		_dprintf("%s: set wds_enable failed\n", ifname);
	}

#else	/* !CONFIG_BCMWL5 */

	if (!foreach_wif(1, NULL, security_on)) return;

	int i;
	int unit;

	i = 10;
	while (pidof("nas") == -1) {
		_dprintf("waiting for nas i=%d\n", i);
		if (--i < 0) {
			syslog(LOG_ERR, "Unable to find nas");
			break;
		}
		sleep(1);
	}
	sleep(5);

	/* the wireless interface must be configured to run NAS */
	wl_ioctl((char *)ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));

	xstart("nas4not", "lan", ifname, "up", "auto",
		nvram_safe_get(wl_nvname("crypto", unit, 0)),	// aes, tkip (aes+tkip ok?)
		nvram_safe_get(wl_nvname("akm", unit, 0)),	// psk (only?)
		nvram_safe_get(wl_nvname("wpa_psk", unit, 0)),	// shared key
		nvram_safe_get(wl_nvname("ssid", unit, 0))	// ssid
	);

#endif /* CONFIG_BCMWL5 */
}
