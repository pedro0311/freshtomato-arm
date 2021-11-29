/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/


#include "rc.h"
#ifdef TCONFIG_BCM7
#include "shared.h"
#endif

#include <ctype.h>
#include <termios.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <time.h>
#include <errno.h>
#include <paths.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <wlutils.h>
#include <bcmdevs.h>
#include <bcmparams.h>

#define SHELL "/bin/sh"
/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"init_debug"


extern struct nvram_tuple router_defaults[];
int restore_defaults_fb = 0;

void
restore_defaults_module(char *prefix)
{
	struct nvram_tuple *t;

	for (t = router_defaults; t->name; t++) {
		if(strncmp(t->name, prefix, sizeof(prefix))!=0) continue;
		nvram_set(t->name, t->value);
	}
}
#ifdef TCONFIG_BCM7
extern struct nvram_tuple bcm4360ac_defaults[];
extern struct nvram_tuple r8000_params[];

static void set_bcm4360ac_vars(void)
{
	struct nvram_tuple *t;

	/* Restore defaults */
	dbg("Restoring bcm4360ac vars...\n");
	for (t = bcm4360ac_defaults; t->name; t++) {
		if (!nvram_get(t->name))
			nvram_set(t->name, t->value);
	}
}

static void set_r8000_vars(void)
{
	struct nvram_tuple *t;

	/* Restore defaults */
	dbg("Restoring r8000 vars...\n");
	for (t = r8000_params; t->name; t++) {
		if (!nvram_get(t->name))
			nvram_set(t->name, t->value);
	}
}

void
bsd_defaults(void)
{
	char extendno_org[14];
	int ext_num;
	char ext_commit_str[8];
	struct nvram_tuple *t;

	dbg("Restoring bsd settings...\n");

	if (!strlen(nvram_safe_get("extendno_org")) || nvram_match("extendno_org", nvram_safe_get("extendno")))
		return;

	strcpy(extendno_org, nvram_safe_get("extendno_org"));
	if (!strlen(extendno_org) || sscanf(extendno_org, "%d-g%s", &ext_num, ext_commit_str) != 2)
		return;


	for (t = router_defaults; t->name; t++)
		if (strstr(t->name, "bsd"))
			nvram_set(t->name, t->value);
}
#endif


static void
restore_defaults(void)
{
	struct nvram_tuple *t;
	int restore_defaults = 0;
	struct sysinfo info;

	/* Restore defaults if told to or OS has changed */
	if (!restore_defaults)
		restore_defaults = !nvram_match("restore_defaults", "0");

	if (restore_defaults)
		fprintf(stderr, "\n## Restoring defaults... ##\n");

	restore_defaults_fb = restore_defaults;

	/* Restore defaults */
	for (t = router_defaults; t->name; t++) {
		if (restore_defaults || !nvram_get(t->name)) {
			nvram_set(t->name, t->value);
		}
	}

	nvram_set("os_name", "linux");
	nvram_set("os_version", tomato_version);
	nvram_set("os_date", tomato_buildtime);

	/* Adjust et and wl thresh value after reset (for wifi-driver and et_linux.c) */
	if (restore_defaults) {
		memset(&info, 0, sizeof(struct sysinfo));
		sysinfo(&info);
		if (info.totalram <= (TOMATO_RAM_LOW_END * 1024)) { /* Router with less than 50 MB RAM */
			/* Set to 512 as long as onboard memory <= 50 MB RAM */
			nvram_set("wl_txq_thresh", "512");
			nvram_set("et_txq_thresh", "512");	
		}
		else if (info.totalram <= (TOMATO_RAM_MID_END * 1024)) { /* Router with less than 100 MB RAM */
			nvram_set("wl_txq_thresh", "1024");
			nvram_set("et_txq_thresh", "1536");
		}
		else { /* Router with more than 100 MB RAM */
			nvram_set("wl_txq_thresh", "1024");
			nvram_set("et_txq_thresh", "3300");
		}
	}
}

/* assign none-exist value */
void
wl_defaults(void)
{
	struct nvram_tuple *t;
	char prefix[]="wlXXXXXX_", tmp[100], tmp2[100];
	char word[256], *next;
	int unit;
	char wlx_vifnames[64], wl_vifnames[64], lan_ifnames[128];

	memset(wlx_vifnames, 0, sizeof(wlx_vifnames));
	memset(wl_vifnames, 0, sizeof(wl_vifnames));
	memset(lan_ifnames, 0, sizeof(lan_ifnames));

	dbg("Restoring wireless vars ...\n");

	if (!nvram_get("wl_country_code"))
		nvram_set("wl_country_code", "");

	unit = 0;
	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	dbg("Restoring wireless vars - in progress ...\n");

		for (t = router_defaults; t->name; t++) {
#ifdef CONFIG_BCMWL5
			if (!strncmp(t->name, "wl", 2) && strncmp(t->name, "wl_", 3) && strncmp(t->name, "wlc", 3) && !strcmp(&t->name[4], "nband"))
				nvram_set(t->name, t->value);
#endif
			if (strncmp(t->name, "wl_", 3)!=0) continue;
#ifdef CONFIG_BCMWL5
			if (!strcmp(&t->name[3], "nband") && nvram_match(strcat_r(prefix, &t->name[3], tmp), "-1"))
				nvram_set(strcat_r(prefix, &t->name[3], tmp), t->value);
#endif
			if (!nvram_get(strcat_r(prefix, &t->name[3], tmp))) {
				/* Add special default value handle here */
#ifdef TCONFIG_EMF
				/* Wireless IGMP Snooping */
				if (strncmp(&t->name[3], "igs", sizeof("igs")) == 0) {
					char *value = nvram_get(strcat_r(prefix, "wmf_bss_enable", tmp2));
					nvram_set(tmp, (value && *value) ? value : t->value);
				} else
#endif
					nvram_set(tmp, t->value);
			}
		}

		unit++;
	}
	dbg("Restoring wireless vars - done ...\n");
}

static int fatalsigs[] = {
	SIGILL,
	SIGABRT,
	SIGFPE,
	SIGPIPE,
	SIGBUS,
	SIGSYS,
	SIGTRAP,
	SIGPWR
};

static int initsigs[] = {
	SIGHUP,
	SIGUSR1,
	SIGUSR2,
	SIGINT,
	SIGQUIT,
	SIGALRM,
	SIGTERM
};

static char *defenv[] = {
	"TERM=vt100",
	"HOME=/",
	"PATH=/usr/bin:/bin:/usr/sbin:/sbin",
	"SHELL=" SHELL,
	"USER=root",
	NULL
};

/* Set terminal settings to reasonable defaults */
static void set_term(int fd)
{
	struct termios tty;

	tcgetattr(fd, &tty);

	/* set control chars */
	tty.c_cc[VINTR]  = 3;	/* C-c */
	tty.c_cc[VQUIT]  = 28;	/* C-\ */
	tty.c_cc[VERASE] = 127; /* C-? */
	tty.c_cc[VKILL]  = 21;	/* C-u */
	tty.c_cc[VEOF]   = 4;	/* C-d */
	tty.c_cc[VSTART] = 17;	/* C-q */
	tty.c_cc[VSTOP]  = 19;	/* C-s */
	tty.c_cc[VSUSP]  = 26;	/* C-z */

	/* use line dicipline 0 */
	tty.c_line = 0;

	/* Make it be sane */
	tty.c_cflag &= CBAUD|CBAUDEX|CSIZE|CSTOPB|PARENB|PARODD;
	tty.c_cflag |= CREAD|HUPCL|CLOCAL;


	/* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;

	/* output modes */
	tty.c_oflag = OPOST | ONLCR;

	/* local modes */
	tty.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;

	tcsetattr(fd, TCSANOW, &tty);
}

static int console_init(void)
{
	int fd;

	/* Clean up */
	ioctl(0, TIOCNOTTY, 0);
	close(0);
	close(1);
	close(2);
	setsid();

	/* Reopen console */
	if ((fd = open(_PATH_CONSOLE, O_RDWR)) < 0) {
		/* Avoid debug messages is redirected to socket packet if no exist a UART chip, added by honor, 2003-12-04 */
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
		perror(_PATH_CONSOLE);
		return errno;
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	ioctl(0, TIOCSCTTY, 1);
	tcsetpgrp(0, getpgrp());
	set_term(0);

	return 0;
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
static int waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

static pid_t run_shell(int timeout, int nowait)
{
	pid_t pid;
	int sig;

	/* Wait for user input */
	if (waitfor(STDIN_FILENO, timeout) <= 0) return 0;

	switch (pid = fork()) {
	case -1:
		perror("fork");
		return 0;
	case 0:
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Reopen console */
		console_init();
		printf("\n\nFreshTomato %s\n\n", tomato_version);

		/* Now run it.  The new program will take over this PID,
		 * so nothing further in init.c should be run. */
		execve(SHELL, (char *[]) { SHELL, NULL }, defenv);

		/* We're still here?  Some error happened. */
		perror(SHELL);
		exit(errno);
	default:
		if (nowait) {
			return pid;
		}
		else {
			waitpid(pid, NULL, 0);
			return 0;
		}
	}
}

int console_main(int argc, char *argv[])
{
	for (;;) run_shell(0, 0);

	return 0;
}

static void shutdn(int rb)
{
	unsigned int i;
	int act;
	sigset_t ss;

	logmsg(LOG_DEBUG, "*** %s: shutdn rb=%d", __FUNCTION__, rb);

	sigemptyset(&ss);
	for (i = 0; i < sizeof(fatalsigs) / sizeof(fatalsigs[0]); i++)
		sigaddset(&ss, fatalsigs[i]);
	for (i = 0; i < sizeof(initsigs) / sizeof(initsigs[0]); i++)
		sigaddset(&ss, initsigs[i]);
	sigprocmask(SIG_BLOCK, &ss, NULL);

	for (i = 30; i > 0; --i) {
		if (((act = check_action()) == ACT_IDLE) || (act == ACT_REBOOT)) break;
		logmsg(LOG_DEBUG, "*** %s: busy with %d. Waiting before shutdown... %d", __FUNCTION__, act, i);
		sleep(1);
	}
	set_action(ACT_REBOOT);

	/* Disconnect pppd - need this for PPTP/L2TP to finish gracefully */
	killall("xl2tpd", SIGTERM);
	killall("pppd", SIGTERM);

	logmsg(LOG_DEBUG, "*** %s: TERM", __FUNCTION__);
	kill(-1, SIGTERM);
	sleep(3);
	sync();

	logmsg(LOG_DEBUG, "*** %s: KILL", __FUNCTION__);
	kill(-1, SIGKILL);
	sleep(1);
	sync();

	// umount("/jffs");	/* may hang if not */
	/* routers without JFFS will never reboot
	 * with above call on presseed reset,
	 * let's try eval code here
	 */
	eval("umount", "-f", "/jffs");
	sleep(1);

	if (rb != -1) {
		led(LED_WLAN, LED_OFF);
		if (rb == 0) {
			for (i = 4; i > 0; --i) {
				led(LED_DMZ, LED_ON);
				led(LED_WHITE, LED_ON);
				usleep(250000);
				led(LED_DMZ, LED_OFF);
				led(LED_WHITE, LED_OFF);
				usleep(250000);
			}
		}
	}

	reboot(rb ? RB_AUTOBOOT : RB_HALT_SYSTEM);

	do {
		sleep(1);
	} while (1);
}

static void handle_fatalsigs(int sig)
{
	logmsg(LOG_DEBUG, "*** %s: fatal sig=%d", __FUNCTION__, sig);
	shutdn(-1);
}

/* Fixed the race condition & incorrect code by using sigwait()
 * instead of pause(). But SIGCHLD is a problem, since other
 * code: 1) messes with it and 2) depends on CHLD being caught so
 * that the pid gets immediately reaped instead of left a zombie.
 * Pidof still shows the pid, even though it's in zombie state.
 * So this SIGCHLD handler reaps and then signals the mainline by
 * raising ALRM.
 */
static void handle_reap(int sig)
{
	chld_reap(sig);
	raise(SIGALRM);
}

static int check_nv(const char *name, const char *value)
{
	const char *p;
	if (!nvram_match("manual_boot_nv", "1")) {
		if (((p = nvram_get(name)) == NULL) || (strcmp(p, value) != 0)) {
			logmsg(LOG_DEBUG, "*** %s: error: critical variable %s is invalid. Resetting", __FUNCTION__, name);
			nvram_set(name, value);
			return 1;
		}
	}
	return 0;
}

#if 0  /* not used right now, but could be useful!  */
static int invalid_mac(const char *mac)
{
	if (!mac || !(*mac) || strncasecmp(mac, "00:90:4c", 8) == 0)
		return 1;

	int i = 0, s = 0;
	while (*mac) {
		if (isxdigit(*mac)) {
			i++;
		} else if (*mac == ':') {
			if (i == 0 || i / 2 - 1 != s)
				break;
			++s;
		} else {
			s = -1;
		}
		++mac;
	}
	return !(i == 12 && s == 5);
}

static int find_sercom_mac_addr(void)
{
	FILE *fp;
	char m[6], s[18];

	sprintf(s, MTD_DEV(%dro), 0);
	if ((fp = fopen(s, "rb"))) {
		fseek(fp, 0x1ffa0, SEEK_SET);
		fread(m, sizeof(m), 1, fp);
		fclose(fp);
		sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",
			m[0], m[1], m[2], m[3], m[4], m[5]);
		nvram_set("et0macaddr", s);
		return !invalid_mac(s);
	}
	return 0;
}
#endif /* not used right now, but could be useful!  */

static int init_vlan_ports(void)
{
	int dirty = 0;
	int model = get_model();

	char vlanports[] = "vlanXXXXports";
	char vlanhw[] = "vlanXXXXhwname";
	char vlanvid[] = "vlanXXXXvid";
	char nvvalue[8] = { 0 };
	int num;
	const char *ports, *hwname, *vid;

	/* FreshTomato: check and prepare nvram VLAN values before we start (vlan mapping) */
	for (num = 0; num < TOMATO_VLANNUM; num ++) {
		/* get vlan infos from nvram */
		snprintf(vlanports, sizeof(vlanports), "vlan%dports", num);
		snprintf(vlanhw, sizeof(vlanhw), "vlan%dhwname", num);
		snprintf(vlanvid, sizeof(vlanvid), "vlan%dvid", num);

		hwname = nvram_get(vlanhw);
		ports = nvram_get(vlanports);

		/* check if we use vlanX */
		if ((hwname && strlen(hwname)) || (ports && strlen(ports))) {
			vid = nvram_get(vlanvid);
			if ((vid == NULL) ||
			    (vid && !strlen(vid))) { /* create nvram vlanXvid if missing, we need it! (default ex. Vlan 4 --> Vid 4) */
				snprintf(nvvalue, sizeof(nvvalue), "%d", num);
				nvram_set(vlanvid, nvvalue);
			}
		}
	}

	switch (model) {
#ifdef CONFIG_BCMWL6A
	case MODEL_R6250:
	case MODEL_R6300v2:
		dirty |= check_nv("vlan1ports", "3 2 1 0 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_RTAC56U:
	case MODEL_DIR868L:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_R6400:
	case MODEL_R6400v2:
	case MODEL_R6700v1:
	case MODEL_R6700v3:
	case MODEL_R6900:
	case MODEL_R7000:
	case MODEL_XR300:
	case MODEL_RTN18U:
	case MODEL_RTAC66U_B1: /* also for RT-N66U_C1 and RT-AC1750_B1 */
	case MODEL_RTAC67U: /* also for RT-AC1900U */
	case MODEL_RTAC68U:
	case MODEL_RTAC68UV3:
	case MODEL_RTAC1900P:
#ifdef TCONFIG_BCM7
	case MODEL_RTAC3200:
#endif
	case MODEL_AC15:
	case MODEL_AC18:
	case MODEL_F9K1113v2_20X0:
	case MODEL_F9K1113v2:
	case MODEL_WS880:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
#ifdef TCONFIG_BCM7
	case MODEL_R8000:
		dirty |= check_nv("vlan1ports", "3 2 1 0 8*");
		dirty |= check_nv("vlan2ports", "4 8");
		break;
#endif
	case MODEL_EA6350v1:
	case MODEL_EA6400:
	case MODEL_EA6700:
	case MODEL_WZR1750:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_EA6900:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_R1D:
		dirty |= check_nv("vlan1ports", "0 2 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
#endif /* CONFIG_BCMWL6A */
	}

	return dirty;
}

static void check_bootnv(void)
{
	int dirty;
	int model;

	model = get_model();
	dirty = check_nv("wl0_leddc", "0x640000") | check_nv("wl1_leddc", "0x640000");

	switch (model) {
#ifdef CONFIG_BCMWL6A
	case MODEL_R6400v2:
	case MODEL_R6700v3:
	case MODEL_XR300:
		nvram_unset("et1macaddr");
		nvram_unset("et2macaddr");
		nvram_unset("et3macaddr");
		dirty |= check_nv("wl0_ifname", "eth1");
		dirty |= check_nv("wl1_ifname", "eth2");
		break;
	case MODEL_R6900:
	case MODEL_R7000:
	case MODEL_R6700v1:
	case MODEL_R6400:
	case MODEL_R6250:
	case MODEL_R6300v2:
		nvram_unset("et1macaddr");
		dirty |= check_nv("wl0_ifname", "eth1");
		dirty |= check_nv("wl1_ifname", "eth2");
		break;
#endif /* CONFIG_BCMWL6A */
#ifdef CONFIG_BCM7
	case MODEL_R8000:
		nvram_unset("et1macaddr");
		dirty |= check_nv("wl0_ifname", "eth2");
		dirty |= check_nv("wl1_ifname", "eth1");
		dirty |= check_nv("wl2_ifname", "eth3");
		break;
#endif
	default:
		/* nothing to do right now */
		break;
	} /* switch (model) */

	dirty |= init_vlan_ports();

	if (dirty) {
		nvram_commit();
		sync();
		dbg("Reboot after check NV params / set VLANS...\n");
		reboot(RB_AUTOBOOT);
		exit(0);
	}
}

static int init_nvram(void)
{
	unsigned long features;
	int model;
	const char *mfr;
	const char *name;
	const char *ver;
	char s[256];

	model = get_model();
	sprintf(s, "%d", model);
	nvram_set("t_model", s);

	mfr = "Broadcom";
	name = NULL;
	ver = NULL;
	features = 0;
	switch (model) {

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108

#ifdef CONFIG_BCMWL6A
	case MODEL_RTN18U:
		mfr = "Asus";
		name = "RT-N18U";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0");
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("0:ccode", "SG");

			/* modify/adjust 2,4 GHz WiFi TX parameter (taken from Asus 384 - Aug 2019) */
			nvram_set("0:pa2ga0", "0xFF4A,0x1B7E,0xFCB9");
			nvram_set("0:pa2ga1", "0xFF49,0x1C58,0xFCA2");
			nvram_set("0:pa2ga2", "0xFF4E,0x1B67,0xFCC3");

			/* modify/adjust 2,4 GHz WiFi TX beamforming parameter (taken from Asus 384 - Aug 2019) */
			nvram_set("0:rpcal2g", "0xe3ce");
		}
		set_gpio(GPIO_13, T_HIGH); /* enable gpio 13; make sure it is always on, connected to WiFi IC; otherwise signal will be very weak! */
		break;
	case MODEL_RTAC56U:
		mfr = "Asus";
#ifdef TCONFIG_BCMSMP
		name = "RT-AC56U";
#else
		name = "RT-AC56S"; /* single-core (NOSMP) clone of RT-AC56U */
#endif
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi country settings */
#ifdef TCONFIG_BCMSMP	/* dual core */
			if (nvram_match("odmpid", "RT-AC56R")) { /* check for RT-AC56R first (almost the same like AC56U; adjust a few things) */
				nvram_set("0:ccode", "US");
				nvram_set("1:ccode", "US");
				nvram_set("0:regrev", "0"); /* get 80 MHz channels for RT-AC56R */
				nvram_set("1:regrev", "0");
				nvram_set("ctf_fa_cap", "0"); /* disable fa cap for freshtomato */
			}
			else { /* RT-AC56U */
				nvram_set("0:ccode", "SG");
				nvram_set("1:ccode", "SG");
				nvram_set("0:regrev", "12");
				nvram_set("1:regrev", "12");
			}
#else			/* single core */
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
#endif

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			nvram_set("1:ledbh6", "136"); /* pull up for 5 GHz LED */
			nvram_set("0:ledbh3", "136"); /* pull up for 2.4 GHz LED */
			nvram_unset("1:ledbh10");

			/* power settings for 2,4 GHz WiFi (from dd wrt) */
			nvram_set("0:maxp2ga0", "0x68"); /* old/orig. value = 0x64 */
			nvram_set("0:maxp2ga1", "0x68"); /* old/orig. value = 0x64 */
			nvram_set("0:cck2gpo", "0x1111");
			nvram_set("0:ofdm2gpo", "0x54333333");
			nvram_set("0:mcs2gpo0", "0x3333");
			nvram_set("0:mcs2gpo1", "0x9753");
			nvram_set("0:mcs2gpo2", "0x3333");
			nvram_set("0:mcs2gpo3", "0x9753");
			nvram_set("0:mcs2gpo4", "0x5555");
			nvram_set("0:mcs2gpo5", "0xB755");
			nvram_set("0:mcs2gpo6", "0x5555");
			nvram_set("0:mcs2gpo7", "0xB755");

			/* power settings for 5 GHz WiFi (from dd wrt) */
			nvram_set("1:maxp5ga0", "104,104,104,104"); /* old/orig. value = 100,100,100,100 */
			nvram_set("1:maxp5ga1", "104,104,104,104"); /* old/orig. value = 100,100,100,100 */
			nvram_set("1:mcsbw205glpo", "0xAA864433"); /* old/orig. value = 0x99753333 */
			nvram_set("1:mcsbw405glpo", "0xAA864433");
			nvram_set("1:mcsbw805glpo", "0xAA864433");
			nvram_set("1:mcsbw205gmpo", "0xAA864433");
			nvram_set("1:mcsbw405gmpo", "0xAA864433");
			nvram_set("1:mcsbw805gmpo", "0xAA864433");
			nvram_set("1:mcsbw205ghpo", "0xAA864433");
			nvram_set("1:mcsbw405ghpo", "0xAA864433");
			nvram_set("1:mcsbw805ghpo", "0xAA864433");
		}
		break;
	case MODEL_RTAC67U: /* also for RT-AC1900U */
		mfr = "Asus";
		name = nvram_match("odmpid", "RT-AC67U") ? "RT-AC67U" : "RT-AC1900U";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
		}
		break;
	case MODEL_RTAC68U:
		mfr = "Asus";
		if (nvram_match("cpurev", "c0")) { /* check for C0 CPU first */
			name = "RT-AC68U C1"; /* C1 (and E1; share name) */
		}
		else { /* all the other versions R/P/U ... A1/A2/B1 */
			name = nvram_match("boardrev", "0x1103") ? "RT-AC68P/U B1" : "RT-AC68R/U";
		}
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
		}
		break;
	case MODEL_RTAC68UV3:
		mfr = "Asus";
		name = "RT-AC68U V3";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* for RT-AC68U V3 - let tomato set odmpid right! value is empty? */
			nvram_set("odmpid", "RT-AC68U");
			
		}
		break;
	case MODEL_RTAC1900P: /* also for RT-AC68U B2; both are dual-core 1400 MHz / 800 RAM Router */
		mfr = "Asus";
		name = nvram_match("odmpid", "RT-AC68U") ? "RT-AC68U B2" : "RT-AC1900P";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
		}
		break;
	case MODEL_RTAC66U_B1: /* also for RT-N66U_C1 and RT-AC1750_B1 */
		mfr = "Asus";
		if (nvram_match("odmpid", "RT-N66U_C1"))
			name = "RT-N66U C1";
		else if (nvram_match("odmpid", "RT-AC1750_B1"))
			name = "RT-AC1750 B1";
		else /* default to RT-AC66U B1 */
			name = "RT-AC66U B1";

		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			if (nvram_match("odmpid", "RT-AC1750_B1")) { /* check for RT-AC1750 B1 first (US Retail Edition) */
				nvram_set("0:ccode", "US");
				nvram_set("1:ccode", "US");
				nvram_set("0:regrev", "0"); /* get 80 MHz channels */
				nvram_set("1:regrev", "0");
				nvram_set("ctf_fa_cap", "0"); /* disable fa cap for freshtomato */
			}
			else { /* default for RT-AC66U B1 */
				nvram_set("0:ccode", "SG");
				nvram_set("1:ccode", "SG");
				nvram_set("0:regrev", "12");
				nvram_set("1:regrev", "12");
			}
		}
		break;
#ifdef TCONFIG_BCM7
	case MODEL_RTAC3200:
		mfr = "Asus";
		name = "RT-AC3200";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_invert", "1");
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1 wl2");
			nvram_set("lan_ifnames", "vlan1 eth2 eth1 eth3");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth2 eth1 eth3");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl2_ifname", "eth3");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");
			nvram_set("wl2_vifnames", "wl2.1 wl2.2 wl2.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("1:macaddr", s);			/* fix WL mac for wl0 (1:) - 2,4GHz - eth2 (do not use the same MAC address like for LAN) */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("0:macaddr", s);			/* fix WL mac for wl1 (0:) - 5GHz low (first one) - eth1 */
			nvram_set("wl1_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("2:macaddr", s);			/* fix WL mac for wl2 (2:) - 5GHz high (second one) - eth3 */
			nvram_set("wl2_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			/* wl0 (1:) - 2,4GHz */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("1:ccode", "SG");
			nvram_set("1:regrev", "12");
			/* wl1 (0:) - 5GHz low */
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("0:ccode", "SG");
			nvram_set("0:regrev", "12");
			/* wl2 (2:) - 5GHz high */
			nvram_set("wl2_bw_cap", "7");
			nvram_set("wl2_chanspec", "104/80");
			nvram_set("wl2_channel", "104");
			nvram_set("wl2_nbw","80");
			nvram_set("wl2_nbw_cap","3");
			nvram_set("wl2_nctrlsb", "upper");
			nvram_set("2:ccode", "SG");
			nvram_set("2:regrev", "12");

			bsd_defaults();
			set_bcm4360ac_vars();
		}
		break;
	case MODEL_R8000:
		mfr = "Netgear";
		name = "R8000";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et2");
			nvram_set("vlan2hwname", "et2");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1 wl2");
			nvram_set("lan_ifnames", "vlan1 eth2 eth1 eth3");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth2 eth1 eth3");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl2_ifname", "eth3");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");
			nvram_set("wl2_vifnames", "wl2.1 wl2.2 wl2.3");

			/* GMAC3 variables */
			nvram_set("fwd_cpumap", "d:x:2:169:1 d:l:5:169:1 d:u:5:163:0");
			nvram_set("fwd_wlandevs", "");
			nvram_set("fwddevs", "");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et2macaddr"));	/* get et2 MAC address for LAN */
			nvram_set("et0macaddr", s); 			/* copy et2macaddr to et0macaddr (see also function start_vlan(void) at rc/interface.c */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("1:macaddr", s);			/* fix WL mac for wl0 (1:) - 2,4GHz - eth2 */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("0:macaddr", s);			/* fix WL mac for wl1 (0:) - 5GHz high - eth1 */
			nvram_set("wl1_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("2:macaddr", s);			/* fix WL mac for wl2 (2:) - 5GHz low - eth3 */
			nvram_set("wl2_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			/* wl0 (1:) - 2,4GHz */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("1:ccode", "SG");
			nvram_set("1:regrev", "12");
			/* wl1 (0:) - 5GHz high */
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "100/80");
			nvram_set("wl1_channel", "100");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("0:ccode", "SG");
			nvram_set("0:regrev", "12");
			/* wl2 (2:) - 5GHz low */
			nvram_set("wl2_bw_cap", "7");
			nvram_set("wl2_chanspec", "40/80");
			nvram_set("wl2_channel", "40");
			nvram_set("wl2_nbw","80");
			nvram_set("wl2_nbw_cap","3");
			nvram_set("wl2_nctrlsb", "upper");
			nvram_set("2:ccode", "SG");
			nvram_set("2:regrev", "12");

			/* fix devpath */
			nvram_set("devpath0", "pcie/1/1");
			nvram_set("devpath1", "pcie/2/3");
			nvram_set("devpath2", "pcie/2/4");

			bsd_defaults();
			set_r8000_vars();
		}
		break;
#endif /* TCONFIG_BCM7 */
	case MODEL_AC15:
		mfr = "Tenda";
		name = "AC15";
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* 2.4 GHz and 5 GHz defaults */
			/* let the cfe set the init parameter for wifi modules - nothing to modify/adjust right now */
		}
		break;
	case MODEL_AC18:
		mfr = "Tenda";
		name = "AC18";
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* 2.4 GHz and 5 GHz defaults */
			/* let the cfe set the init parameter for wifi modules - nothing to modify/adjust right now */
		}
		break;
	case MODEL_F9K1113v2_20X0: /* version 2000 and 2010 */
		mfr = "Belkin";
		name = "F9K1113v2";
		features = SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_unset("devpath0"); /* unset devpath, we do not use/need it! */
			nvram_unset("devpath1");
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for wl0 (0:) 5G - eth1 for F9K1113v2 */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for wl1 (1:) 2.4G - eth2 for F9K1113v2 */
			nvram_set("wl1_hwaddr", s);

			/* 5G settings */
			nvram_set("wl0_bw_cap", "7");
			nvram_set("wl0_chanspec", "36/80");
			nvram_set("wl0_channel", "36");
			nvram_set("wl0_nband", "1");
			nvram_set("wl0_nbw","80");
			nvram_set("wl0_nbw_cap","3");
			nvram_set("wl0_nctrlsb", "lower");

			/* 2G settings */
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","6u");
			nvram_set("wl1_channel","6");
			nvram_set("wl1_nband", "2");
			nvram_set("wl1_nbw","40");
			nvram_set("wl1_nctrlsb", "upper");

			/* misc wifi settings */
			nvram_set("wl1_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			nvram_set("wl0_ssid", "FreshTomato50");
			nvram_set("wl1_ssid", "FreshTomato24");

#ifdef TCONFIG_BCMBSD
			/* band steering settings correction, because 5 GHz module is the first one */
			nvram_set("wl1_bsd_steering_policy", "0 5 3 -52 0 110 0x22");
			nvram_set("wl0_bsd_steering_policy", "80 5 3 -82 0 0 0x20");
			nvram_set("wl1_bsd_sta_select_policy", "10 -52 0 110 0 1 1 0 0 0 0x122");
			nvram_set("wl0_bsd_sta_select_policy", "10 -82 0 0 0 1 1 0 0 0 0x20");
			nvram_set("wl1_bsd_if_select_policy", "eth1");
			nvram_set("wl0_bsd_if_select_policy", "eth2");
			nvram_set("wl1_bsd_if_qualify_policy", "0 0x0");
			nvram_set("wl0_bsd_if_qualify_policy", "60 0x0");
#endif /* TCONFIG_BCMBSD */

			/* usb settings */
			nvram_set("usb_ohci", "1");     /* USB 1.1 */
			nvram_set("usb_usb3", "1");     /* USB 3.0 */
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* 2.4GHz module defaults */
			nvram_set("pci/2/1/aa2g", "3");
			nvram_set("pci/2/1/ag0", "0");
			nvram_set("pci/2/1/ag1", "0");
			nvram_set("pci/2/1/antswctl2g", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags2", "0x00001800");
			nvram_set("pci/2/1/boardflags", "0x80001200");
			nvram_set("pci/2/1/bw402gpo", "0x0");
			nvram_set("pci/2/1/bwdup2gpo", "0x0");
			nvram_set("pci/2/1/cck2gpo", "0x5555");
			nvram_set("pci/2/1/cdd2gpo", "0x0");
			nvram_set("pci/2/1/devid", "0x43A9");
			nvram_set("pci/2/1/elna2g", "2");
			nvram_set("pci/2/1/extpagain2g", "3");
			nvram_set("pci/2/1/itt2ga0", "0x20");
			nvram_set("pci/2/1/itt2ga1", "0x20");
			nvram_set("pci/2/1/ledbh0", "11");
			nvram_set("pci/2/1/ledbh1", "11");
			nvram_set("pci/2/1/ledbh2", "11");
			nvram_set("pci/2/1/ledbh3", "11");
			nvram_set("pci/2/1/leddc", "0xFFFF");
			nvram_set("pci/2/1/maxp2ga0", "0x4c");
			nvram_set("pci/2/1/maxp2ga1", "0x4c");
			nvram_set("pci/2/1/maxp2ga2", "0x4c");
			nvram_set("pci/2/1/mcs2gpo0", "0x4444");
			nvram_set("pci/2/1/mcs2gpo1", "0x4444");
			nvram_set("pci/2/1/mcs2gpo2", "0xaaaa");
			nvram_set("pci/2/1/mcs2gpo3", "0xaaaa");
			nvram_set("pci/2/1/mcs2gpo4", "0x6666");
			nvram_set("pci/2/1/mcs2gpo5", "0x6666");
			nvram_set("pci/2/1/mcs2gpo6", "0x6666");
			nvram_set("pci/2/1/mcs2gpo7", "0x6666");
			nvram_set("pci/2/1/ofdm2gpo", "0x33333333");
			nvram_set("pci/2/1/pa2gw0a0", "0xfea8");
			nvram_set("pci/2/1/pa2gw0a1", "0xfeb4");
			nvram_set("pci/2/1/pa2gw1a0", "0x1b2e");
			nvram_set("pci/2/1/pa2gw1a1", "0x1c8a");
			nvram_set("pci/2/1/pa2gw2a0", "0xfa28");
			nvram_set("pci/2/1/pa2gw2a1", "0xfa04");
			nvram_set("pci/2/1/pdetrange2g", "3");
			nvram_set("pci/2/1/phycal_tempdelta", "0");
			nvram_set("pci/2/1/rxchain", "3");
			nvram_set("pci/2/1/sromrev", "8");
			nvram_set("pci/2/1/stbc2gpo", "0x0");
			nvram_set("pci/2/1/tempoffset", "0");
			nvram_set("pci/2/1/temps_hysteresis", "5");
			nvram_set("pci/2/1/temps_period", "5");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/triso2g", "2");
			nvram_set("pci/2/1/tssipos2g", "1");
			nvram_set("pci/2/1/txchain", "3");
			nvram_set("pci/2/1/venid", "0x14E4");
			nvram_set("pci/2/1/xtalfreq", "20000");

			/* 5GHz module defaults */
			nvram_set("pci/1/1/aa5g", "3");
			nvram_set("pci/1/1/aga0", "0");
			nvram_set("pci/1/1/aga1", "0");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags2", "0x00300002");
			nvram_set("pci/1/1/boardflags3", "0x0");
			nvram_set("pci/1/1/boardflags", "0x30000000");
			nvram_set("pci/1/1/devid", "0x43B3");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/epagain5g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
			nvram_set("pci/1/1/ledbh0", "11");
			nvram_set("pci/1/1/ledbh1", "11");
			nvram_set("pci/1/1/ledbh2", "11");
			nvram_set("pci/1/1/ledbh3", "11");
			nvram_set("pci/1/1/ledbh10", "2");
			nvram_set("pci/1/1/leddc", "0xFFFF");
			nvram_set("pci/1/1/maxp5ga0", "54,86,86,86");
			nvram_set("pci/1/1/maxp5ga1", "54,86,86,86");
			nvram_set("pci/1/1/mcsbw205ghpo", "0xDC642000");
			nvram_set("pci/1/1/mcsbw205glpo", "0x0");
			nvram_set("pci/1/1/mcsbw205gmpo", "0xDC862000");
			nvram_set("pci/1/1/mcsbw405ghpo", "0xDC642000");
			nvram_set("pci/1/1/mcsbw405glpo", "0x0");
			nvram_set("pci/1/1/mcsbw405gmpo", "0xDC862000");
			nvram_set("pci/1/1/mcsbw805ghpo", "0xDC642000");
			nvram_set("pci/1/1/mcsbw805glpo", "0x0");
			nvram_set("pci/1/1/mcsbw805gmpo", "0xDC862000");
			nvram_set("pci/1/1/mcsbw1605ghpo", "0");
			nvram_set("pci/1/1/mcsbw1605glpo", "0");
			nvram_set("pci/1/1/mcsbw1605gmpo", "0");
			nvram_set("pci/1/1/mcslr5ghpo", "0");
			nvram_set("pci/1/1/mcslr5glpo", "0");
			nvram_set("pci/1/1/mcslr5gmpo", "0");
			nvram_set("pci/1/1/pa5ga0", "0xff27,0x16e1,0xfd1e,0xff2c,0x1880,0xfcf7,0xff37,0x18fa,0xfcf7,0xff3c,0x18e6,0xfcf2");
			nvram_set("pci/1/1/pa5ga1", "0xff3e,0x19aa,0xfce1,0xff2f,0x190c,0xfce6,0xff2c,0x1875,0xfcfa,0xff3d,0x18f6,0xfcf1");
			nvram_set("pci/1/1/papdcap5g", "0");
			nvram_set("pci/1/1/pdgain5g", "4");
			nvram_set("pci/1/1/pdoffset40ma0", "0x3222");
			nvram_set("pci/1/1/pdoffset40ma1", "0x3222");
			nvram_set("pci/1/1/pdoffset80ma0", "0x0100");
			nvram_set("pci/1/1/pdoffset80ma1", "0x0100");
			nvram_set("pci/1/1/phycal_tempdelta", "0");
			nvram_set("pci/1/1/rxchain", "3");
			nvram_set("pci/1/1/rxgains5gelnagaina0", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina1", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina2", "1");
			nvram_set("pci/1/1/rxgains5ghelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gmelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/1/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/1/1/sar2g", "18");
			nvram_set("pci/1/1/sar5g", "15");
			nvram_set("pci/1/1/sb20in40hrpo", "0");
			nvram_set("pci/1/1/sb20in40lrpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80hr5glpo", "0");
			nvram_set("pci/1/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80lr5glpo", "0");
			nvram_set("pci/1/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "4");
			nvram_set("pci/1/1/tempoffset", "0");
			nvram_set("pci/1/1/temps_hysteresis", "5");
			nvram_set("pci/1/1/temps_period", "5");
			nvram_set("pci/1/1/tempthresh", "120");
			nvram_set("pci/1/1/tssiposslope5g", "1");
			nvram_set("pci/1/1/tworangetssi5g", "0");
			nvram_set("pci/1/1/txchain", "3");
			nvram_set("pci/1/1/venid", "0x14E4");
			nvram_set("pci/1/1/xtalfreq", "40000");
		}
		break;
	case MODEL_F9K1113v2:
		mfr = "Belkin";
		name = "F9K1113v2";
		features = SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("devpath0", "pci/1/1/");
			nvram_set("devpath1", "pci/2/1/");
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for wl0 (0:) 5G - eth1 for F9K1113v2 and/or wl0 (0:) 5G - eth1 for F9K1113v2 */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for wl1 (1:) 2.4G - eth2 for F9K1113v2 and/or wl1 (1:) 2,4G - eth2 for F9K1113v2 */
			nvram_set("wl1_hwaddr", s);

			/* 5G settings */
			nvram_set("wl0_bw_cap", "7");
			nvram_set("wl0_chanspec", "36/80");
			nvram_set("wl0_channel", "36");
			nvram_set("wl0_nband", "1");
			nvram_set("wl0_nbw","80");
			nvram_set("wl0_nbw_cap","3");
			nvram_set("wl0_nctrlsb", "lower");

			/* 2G settings */
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","6u");
			nvram_set("wl1_channel","6");
			nvram_set("wl1_nband", "2");
			nvram_set("wl1_nbw","40");
			nvram_set("wl1_nctrlsb", "upper");

			/* misc wifi settings */
			nvram_set("wl1_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* wifi country settings */
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");

			nvram_set("wl0_ssid", "FreshTomato50");
			nvram_set("wl1_ssid", "FreshTomato24");
			
#ifdef TCONFIG_BCMBSD
			/* band steering settings correction, because 5 GHz module is the first one */
			nvram_set("wl1_bsd_steering_policy", "0 5 3 -52 0 110 0x22");
			nvram_set("wl0_bsd_steering_policy", "80 5 3 -82 0 0 0x20");
			nvram_set("wl1_bsd_sta_select_policy", "10 -52 0 110 0 1 1 0 0 0 0x122");
			nvram_set("wl0_bsd_sta_select_policy", "10 -82 0 0 0 1 1 0 0 0 0x20");
			nvram_set("wl1_bsd_if_select_policy", "eth1");
			nvram_set("wl0_bsd_if_select_policy", "eth2");
			nvram_set("wl1_bsd_if_qualify_policy", "0 0x0");
			nvram_set("wl0_bsd_if_qualify_policy", "60 0x0");
#endif /* TCONFIG_BCMBSD */

			/* usb settings */
			nvram_set("usb_ohci", "1");     /* USB 1.1 */
			nvram_set("usb_usb3", "1");     /* USB 3.0 */
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* 2.4GHz module defaults */
			nvram_set("1:devid", "0x43A9");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:boardflags", "0x80001200");
			nvram_set("1:boardflags2", "0x00001800");
			nvram_set("1:ag0", "0");
			nvram_set("1:ag1", "0");
			nvram_set("1:aa2g", "3");
			nvram_set("1:antswctl2g", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:bwdup2gpo", "0x0");
			nvram_set("1:bw402gpo", "0x0");
			nvram_set("1:cdd2gpo", "0x0");
			nvram_set("1:cck2gpo", "0x5555");
			nvram_set("1:elna2g", "2");
			nvram_set("1:extpagain2g", "3");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:itt2ga0", "0x20");
			nvram_set("1:itt2ga1", "0x20");
			nvram_set("1:maxp2ga0", "0x68");
			nvram_set("1:maxp2ga1", "0x68");
			nvram_set("1:maxp2ga2", "0x68");
			nvram_set("1:mcs2gpo0", "0x4444");
			nvram_set("1:mcs2gpo1", "0x4444");
			nvram_set("1:mcs2gpo2", "0x4444");
			nvram_set("1:mcs2gpo3", "0x4444");
			nvram_set("1:mcs2gpo4", "0x6666");
			nvram_set("1:mcs2gpo5", "0x6666");
			nvram_set("1:mcs2gpo6", "0x6666");
			nvram_set("1:mcs2gpo7", "0x6666");
			nvram_set("1:ofdm2gpo", "0x44444444");
			nvram_set("1:pa2gw0a0", "0xfea8");
			nvram_set("1:pa2gw1a0", "0x1aae");
			nvram_set("1:pa2gw2a0", "0xfa3d");
			nvram_set("1:pa2gw0a1", "0xfeb4");
			nvram_set("1:pa2gw1a1", "0x1c0a");
			nvram_set("1:pa2gw2a1", "0xfa18");
			nvram_set("1:pdetrange2g", "3");
			nvram_set("1:phycal_tempdelta", "0");
			nvram_set("1:rxchain", "3");
			nvram_set("1:sromrev", "8");
			nvram_set("1:stbc2gpo", "0x0");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:temps_period", "5");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:triso2g", "3");
			nvram_set("1:tssipos2g", "1");
			nvram_set("1:txchain", "3");
			nvram_set("1:xtalfreq", "20000");

			/* 5GHz module defaults */
			nvram_set("0:devid", "0x43A2");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:boardflags", "0x30000000");
			nvram_set("0:boardflags2", "0x00200002");
			nvram_set("0:boardflags3", "00");
			nvram_set("0:aga0", "0");
			nvram_set("0:aga1", "0");
			nvram_set("0:aga2", "0");
			nvram_set("0:aa2g", "7");
			nvram_set("0:aa5g", "7");
			nvram_set("0:antswitch", "0");
			nvram_set("0:dot11agduplrpo", "0");
			nvram_set("0:dot11agduphrpo", "0");
			nvram_set("0:epagain5g", "0");
			nvram_set("0:femctrl", "3");
			nvram_set("0:gainctrlsph", "0");
			nvram_set("0:ledbh0", "11");
			nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "11");
			nvram_set("0:ledbh3", "11");
			nvram_set("0:ledbh10", "2");
			nvram_set("0:leddc", "0xFFFF");
			nvram_set("0:maxp5ga0", "64,96,96,96");
			nvram_set("0:maxp5ga1", "64,96,96,96");
			nvram_set("0:maxp5ga2", "64,96,96,96");
			nvram_set("0:mcsbw805ghpo", "0xCC644320");
			nvram_set("0:mcsbw405ghpo", "0xCC644320");
			nvram_set("0:mcsbw805gmpo", "0xEE865420");
			nvram_set("0:mcsbw205gmpo", "0xEE865420");
			nvram_set("0:mcsbw205ghpo", "0xCC644320");
			nvram_set("0:mcsbw405gmpo", "0xEE865420");
			nvram_set("0:mcsbw805glpo", "0");
			nvram_set("0:mcsbw405glpo", "0");
			nvram_set("0:mcslr5gmpo", "0");
			nvram_set("0:mcsbw1605gmpo", "0");
			nvram_set("0:mcsbw1605glpo", "0");
			nvram_set("0:mcsbw1605ghpo", "0");
			nvram_set("0:mcslr5glpo", "0");
			nvram_set("0:mcslr5ghpo", "0");
			nvram_set("0:mcsbw205glpo", "0");
			nvram_set("0:pa5ga0", "0xff3f,0x19ee,0xfcdc,0xff25,0x17e0,0xfcff,0xff3a,0x1928,0xfcf0,0xff2d,0x1905,0xfce7");
			nvram_set("0:pa5ga1", "0xff30,0x18ff,0xfce7,0xff34,0x191a,0xfce9,0xff24,0x17a3,0xfd0a,0xff30,0x1913,0xfcea");
			nvram_set("0:pa5ga2", "0xff32,0x18c1,0xfcf4,0xff36,0x18ed,0xfcf3,0xff38,0x198f,0xfce0,0xff3a,0x19b9,0xfce4");
			nvram_set("0:papdcap5g", "0");
			nvram_set("0:pdoffset80ma0", "0x0100");
			nvram_set("0:pdoffset80ma1", "0x0100");
			nvram_set("0:pdoffset80ma2", "0x0100");
			nvram_set("0:pdoffset40ma0", "0x3222");
			nvram_set("0:pdoffset40ma1", "0x3222");
			nvram_set("0:pdoffset40ma2", "0x3222");
			nvram_set("0:pdgain5g", "4");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rxchain", "7");
			nvram_set("0:rxgains5ghelnagaina0", "2");
			nvram_set("0:rxgains5ghelnagaina1", "2");
			nvram_set("0:rxgains5ghelnagaina2", "3");
			nvram_set("0:rxgains5gelnagaina0", "1");
			nvram_set("0:rxgains5gelnagaina1", "1");
			nvram_set("0:rxgains5gelnagaina2", "1");
			nvram_set("0:rxgains5ghtrelnabypa0", "1");
			nvram_set("0:rxgains5ghtrelnabypa1", "1");
			nvram_set("0:rxgains5ghtrelnabypa2", "1");
			nvram_set("0:rxgains5ghtrisoa0", "5");
			nvram_set("0:rxgains5ghtrisoa1", "4");
			nvram_set("0:rxgains5ghtrisoa2", "4");
			nvram_set("0:rxgains5gmelnagaina0", "2");
			nvram_set("0:rxgains5gmelnagaina1", "2");
			nvram_set("0:rxgains5gmelnagaina2", "3");
			nvram_set("0:rxgains5gmtrelnabypa0", "1");
			nvram_set("0:rxgains5gmtrelnabypa1", "1");
			nvram_set("0:rxgains5gmtrelnabypa2", "1");
			nvram_set("0:rxgains5gmtrisoa0", "5");
			nvram_set("0:rxgains5gmtrisoa1", "4");
			nvram_set("0:rxgains5gmtrisoa2", "4");
			nvram_set("0:rxgains5gtrisoa0", "7");
			nvram_set("0:rxgains5gtrisoa1", "6");
			nvram_set("0:rxgains5gtrisoa2", "5");
			nvram_set("0:sar2g", "18");
			nvram_set("0:sar5g", "15");
			nvram_set("0:sb20in80and160lr5gmpo", "0");
			nvram_set("0:sb20in80and160hr5gmpo", "0");
			nvram_set("0:sb20in40lrpo", "0");
			nvram_set("0:sb40and80lr5gmpo", "0");
			nvram_set("0:sb40and80hr5gmpo", "0");
			nvram_set("0:sb20in80and160lr5glpo", "0");
			nvram_set("0:sb20in80and160lr5ghpo", "0");
			nvram_set("0:sb40and80lr5glpo", "0");
			nvram_set("0:subband5gver", "4");
			nvram_set("0:sb40and80lr5ghpo", "0");
			nvram_set("0:sb20in80and160hr5glpo", "0");
			nvram_set("0:sb20in80and160hr5ghpo", "0");
			nvram_set("0:sb20in40hrpo", "0");
			nvram_set("0:sb40and80hr5glpo", "0");
			nvram_set("0:sromrev", "11");
			nvram_set("0:tssiposslope5g", "1");
			nvram_set("0:tworangetssi5g", "0");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:sb40and80hr5ghpo", "0");
			nvram_set("0:rxgains5gtrelnabypa0", "1");
			nvram_set("0:rxgains5gtrelnabypa1", "1");
			nvram_set("0:rxgains5gtrelnabypa2", "1");
			nvram_set("0:temps_hysteresis", "55");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:txchain", "7");
			nvram_set("0:xtalfreq", "40000");
		}
		break;
	case MODEL_R6250:
		mfr = "Netgear";
		name = "R6250";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* disable second *fake* LAN interface */
			nvram_unset("et1macaddr");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz defaults */
			nvram_set("pci/1/1/aa2g", "3");
			nvram_set("pci/1/1/ag0", "2");
			nvram_set("pci/1/1/ag1", "2");
			nvram_set("pci/1/1/ag2", "255");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/antswctl2g", "0");
			nvram_set("pci/1/1/boardflags", "0x80001200");
			nvram_set("pci/1/1/boardflags2", "0x9800");
			nvram_set("pci/1/1/boardtype", "0x62b");
			nvram_set("pci/1/1/boardvendor", "0x14e4");
			nvram_set("pci/1/1/ccd2gpo", "0");
			nvram_set("pci/1/1/cck2gpo", "0");
//			nvram_set("pci/1/1/ccode", "EU");
			nvram_set("pci/1/1/devid", "0x43a9");
			nvram_set("pci/1/1/elna2g", "2");
			nvram_set("pci/1/1/extpagain2g", "3");
			nvram_set("pci/1/1/maxp2ga0", "0x66");
			nvram_set("pci/1/1/maxp2ga1", "0x66");
			nvram_set("pci/1/1/mcs2gpo0", "0x4000");
			nvram_set("pci/1/1/mcs2gpo1", "0xCA86");
			nvram_set("pci/1/1/mcs2gpo2", "0x4000");
			nvram_set("pci/1/1/mcs2gpo3", "0xCA86");
			nvram_set("pci/1/1/mcs2gpo4", "0x7422");
			nvram_set("pci/1/1/mcs2gpo5", "0xEDB9");
			nvram_set("pci/1/1/mcs2gpo6", "0x7422");
			nvram_set("pci/1/1/mcs2gpo7", "0xEDB9");
			nvram_set("pci/1/1/bw402gpo", "0x1");
			nvram_set("pci/1/1/ofdm5gpo", "0");
			nvram_set("pci/1/1/ofdm2gpo", "0xA8640000");
			nvram_set("pci/1/1/ofdm5glpo", "0");
			nvram_set("pci/1/1/ofdm5ghpo", "0");
			nvram_set("pci/1/1/opo", "68");
			nvram_set("pci/1/1/pa2gw0a0", "0xff15");
			nvram_set("pci/1/1/pa2gw0a1", "0xff15");
			nvram_set("pci/1/1/pa2gw1a0", "0x1870");
			nvram_set("pci/1/1/pa2gw1a1", "0x1870");
			nvram_set("pci/1/1/pa2gw2a0", "0xfad3");
			nvram_set("pci/1/1/pa2gw2a1", "0xfad3");
			nvram_set("pci/1/1/pdetrange2g", "3");
			nvram_set("pci/1/1/ledbh12", "11");
			nvram_set("pci/1/1/ledbh0", "255");
			nvram_set("pci/1/1/ledbh1", "255");
			nvram_set("pci/1/1/ledbh2", "255");
			nvram_set("pci/1/1/ledbh3", "131");
			nvram_set("pci/1/1/leddc", "65535");
//			nvram_set("pci/1/1/regrev", "22");
			nvram_set("pci/1/1/rxchain", "3");
			nvram_set("pci/1/1/sromrev", "8");
			nvram_set("pci/1/1/stbc2gpo", "0");
			nvram_set("pci/1/1/triso2g", "4");
			nvram_set("pci/1/1/tssipos2g", "1");
			nvram_set("pci/1/1/tempthresh", "120");
			nvram_set("pci/1/1/tempoffset", "0");
			nvram_set("pci/1/1/txchain", "3");
			nvram_set("pci/1/1/venid", "0x14e4");

			/* 5 GHz module defaults */
			nvram_set("pci/2/1/aa5g", "7");
			nvram_set("pci/2/1/aga0", "71");
			nvram_set("pci/2/1/aga1", "133");
			nvram_set("pci/2/1/aga2", "133");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x10001000");
			nvram_set("pci/2/1/boardflags2", "0x2");
			nvram_set("pci/2/1/boardflags3", "0x0");
			nvram_set("pci/2/1/boardvendor", "0x14e4");
//			nvram_set("pci/2/1/ccode", "Q1");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/femctrl", "6");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/maxp5ga0", "72,72,94,94");
			nvram_set("pci/2/1/maxp5ga1", "72,72,94,94");
			nvram_set("pci/2/1/maxp5ga2", "72,72,94,94");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0xFC652000");
			nvram_set("pci/2/1/mcsbw205glpo", "0xEC200000");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xEC200000");
			nvram_set("pci/2/1/mcsbw405ghpo", "0xFC764100");
			nvram_set("pci/2/1/mcsbw405glpo", "0xEC30000");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xEC300000");
			nvram_set("pci/2/1/mcsbw805ghpo", "0xFDA86420");
			nvram_set("pci/2/1/mcsbw805glpo", "0xFCA86400");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xFDA86420");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/ofdm5glpo", "0");
			nvram_set("pci/2/1/ofdm5ghpo", "0xB975300");
			nvram_set("pci/2/1/pa5ga0", "0xff7a,0x16a9,0xfd4b,0xff6e,0x1691,0xfd47,0xff7e,0x17b8,0xfd37,0xff82,0x17fb,0xfd3a");
			nvram_set("pci/2/1/pa5ga1", "0xff66,0x1519,0xfd65,0xff72,0x15ff,0xfd56,0xff7f,0x16ee,0xfd4b,0xffad,0x174b,0xfd81");
			nvram_set("pci/2/1/pa5ga2", "0xff76,0x168e,0xfd50,0xff75,0x16d0,0xfd4b,0xff86,0x17fe,0xfd39,0xff7e,0x1810,0xfd31");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdgain5g", "10");
			nvram_set("pci/2/1/pdoffset40ma0", "12834");
			nvram_set("pci/2/1/pdoffset40ma1", "12834");
			nvram_set("pci/2/1/pdoffset40ma2", "12834");
			nvram_set("pci/2/1/pdoffset80ma0", "256");
			nvram_set("pci/2/1/pdoffset80ma1", "256");
			nvram_set("pci/2/1/pdoffset80ma2", "256");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
//			nvram_set("pci/2/1/regrev", "27");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
			nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
			nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "3");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "7");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "7");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "7");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "15");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "15");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "15");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "7");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "7");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "7");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "15");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "15");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "15");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "6");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/venid", "0x14e4");
			nvram_set("pci/2/1/xtalfreq", "65535");
		}
		break;
	case MODEL_R6300v2:
		mfr = "Netgear";
		name = "R6300v2";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_invert", "1");
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* disable second *fake* LAN interface */
			nvram_unset("et1macaddr");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz defaults */
			nvram_set("pci/1/1/ag0", "0");
			nvram_set("pci/1/1/ag1", "0");
			nvram_set("pci/1/1/ag2", "0");
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/antswctl2g", "0");
			nvram_set("pci/1/1/boardflags", "0x80003200");
			nvram_set("pci/1/1/boardflags2", "0x4100000");
			nvram_set("pci/1/1/boardvendor", "0x14e4");
//			nvram_set("pci/1/1/ccode", "Q2");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/devid", "0x4332");
			nvram_set("pci/1/1/elna2g", "2");
			nvram_set("pci/1/1/extpagain2g", "1");
			nvram_set("pci/1/1/maxp2ga0", "0x66");
			nvram_set("pci/1/1/maxp2ga1", "0x66");
			nvram_set("pci/1/1/maxp2ga2", "0x66");
			nvram_set("pci/1/1/mcs32po", "0x8");
			nvram_set("pci/1/1/mcsbw20ul2gpo", "0xCA862222");
			nvram_set("pci/1/1/mcsbw202gpo", "0xCA862222");
			nvram_set("pci/1/1/mcsbw402gpo", "0xECA86222");
			nvram_set("pci/1/1/pa2gw0a0", "0xFEB4");
			nvram_set("pci/1/1/pa2gw0a1", "0xFEBC");
			nvram_set("pci/1/1/pa2gw0a2", "0xFEA9");
			nvram_set("pci/1/1/pa2gw2a0", "0xF9EE");
			nvram_set("pci/1/1/pa2gw2a1", "0xFA05");
			nvram_set("pci/1/1/pa2gw2a2", "0xFA03");
			nvram_set("pci/1/1/pa2gw1a0", "0x1A37");
			nvram_set("pci/1/1/pa2gw1a1", "0x1A0B");
			nvram_set("pci/1/1/pa2gw1a2", "0x19F2");
			nvram_set("pci/1/1/pdetrange2g", "3");
//			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/1/1/rxchain", "7");
			nvram_set("pci/1/1/ledbh12", "11");
			nvram_set("pci/1/1/ledbh0", "11");
			nvram_set("pci/1/1/ledbh2", "14");
			nvram_set("pci/1/1/ledbh3", "1");
			nvram_set("pci/1/1/leddc", "0xFFFF");
			nvram_set("pci/1/1/legofdmbw202gpo", "0xCA862222");
			nvram_set("pci/1/1/legofdmbw20ul2gpo", "0xCA862222");
			nvram_set("pci/1/1/legofdm40duppo", "0x0");
			nvram_set("pci/1/1/sromrev", "9");
			nvram_set("pci/1/1/triso2g", "3");
			nvram_set("pci/1/1/tempthresh", "120");
			nvram_set("pci/1/1/tempoffset", "0");
			nvram_set("pci/1/1/tssipos2g", "1");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14e4");
			nvram_set("pci/1/1/xtalfreq", "20000");

			/* 5 GHz module defaults */
			nvram_set("pci/2/1/aa5g", "7");
			nvram_set("pci/2/1/aga0", "71");
			nvram_set("pci/2/1/aga1", "133");
			nvram_set("pci/2/1/aga2", "133");
			nvram_set("pci/2/1/agbg0", "71");
			nvram_set("pci/2/1/agbg1", "71");
			nvram_set("pci/2/1/agbg2", "133");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x0");
			nvram_set("pci/2/1/boardnum", "20771");
			nvram_set("pci/2/1/boardrev", "0x1402");
			nvram_set("pci/2/1/boardvendor", "0x14e4");
			nvram_set("pci/2/1/boardtype", "0x621");
//			nvram_set("pci/2/1/ccode", "Q2");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/maxp5ga0", "102,102,102,102");
			nvram_set("pci/2/1/maxp5ga1", "102,102,102,102");
			nvram_set("pci/2/1/maxp5ga2", "102,102,102,102");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw205glpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405ghpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw405glpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw805ghpo", "0xFEA86400");
			nvram_set("pci/2/1/mcsbw805glpo", "0xFEA86400");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xFEA86400");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/pa5ga0", "0xFF39,0x1A55,0xFCC7,0xFF50,0x1AD0,0xFCE0,0xFF50,0x1B6F,0xFCD0,0xFF58,0x1BB9,0xFCD0");
			nvram_set("pci/2/1/pa5ga1", "0xFF36,0x1AAD,0xFCBD,0xFF50,0x1AF7,0xFCE0,0xFF50,0x1B5B,0xFCD8,0xFF58,0x1B8F,0xFCD0");
			nvram_set("pci/2/1/pa5ga2", "0xFF40,0x1A1F,0xFCDA,0xFF48,0x1A5D,0xFCE8,0xFF35,0x1A2D,0xFCCA,0xFF3E,0x1A2B,0xFCD0");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
//			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
			nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
			nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "1");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "1");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "1");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "2");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "2");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "2");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "2");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/venid", "0x14e4");
			nvram_set("pci/2/1/xtalfreq", "40000");
		}
		break;
	case MODEL_R6400:
		mfr = "Netgear";
		name = "R6400";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* disable second *fake* LAN interface */
			nvram_unset("et1macaddr");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz defaults */
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/ag0", "0");
			nvram_set("pci/1/1/ag1", "0");
			nvram_set("pci/1/1/ag2", "0");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/antswctl2g", "0");
			nvram_set("pci/1/1/boardflags", "0x80003200");
			nvram_set("pci/1/1/boardflags2", "0x4100000");
			nvram_set("pci/1/1/boardvendor", "0x14e4");
//			nvram_set("pci/1/1/ccode", "Q2");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/devid", "0x4332");
			nvram_set("pci/1/1/elna2g", "2");
			nvram_set("pci/1/1/eu_edthresh1g", "-62");
			nvram_set("pci/1/1/extpagain2g", "1");
			nvram_set("pci/1/1/maxp2ga0", "0x60");
			nvram_set("pci/1/1/maxp2ga1", "0x60");
			nvram_set("pci/1/1/maxp2ga2", "0x60");
			nvram_set("pci/1/1/mcsbw20ul2gpo", "0x86522222");
			nvram_set("pci/1/1/mcsbw202gpo", "0x86522222");
			nvram_set("pci/1/1/mcsbw402gpo", "0xEEEEEEEE");
			nvram_set("pci/1/1/mcs32po", "0x8");
			nvram_set("pci/1/1/pa2gw0a0", "0xfe8c");
			nvram_set("pci/1/1/pa2gw0a1", "0xfea3");
			nvram_set("pci/1/1/pa2gw0a2", "0xfe94");
			nvram_set("pci/1/1/pa2gw1a0", "0x1950");
			nvram_set("pci/1/1/pa2gw1a1", "0x18f7");
			nvram_set("pci/1/1/pa2gw1a2", "0x192c");
			nvram_set("pci/1/1/pa2gw2a0", "0xf9f1");
			nvram_set("pci/1/1/pa2gw2a1", "0xfa2c");
			nvram_set("pci/1/1/pa2gw2a2", "0xfa17");
			nvram_set("pci/1/1/pdetrange2g", "3");
			nvram_set("pci/1/1/ledbh12", "11");
			nvram_set("pci/1/1/ledbh0", "11");
			nvram_set("pci/1/1/ledbh1", "11");
			nvram_set("pci/1/1/ledbh2", "11");
			nvram_set("pci/1/1/ledbh3", "11");
			nvram_set("pci/1/1/leddc", "0xFFFF");
			nvram_set("pci/1/1/legofdmbw202gpo", "0x64200000");
			nvram_set("pci/1/1/legofdmbw20ul2gpo", "0x64200000");
			nvram_set("pci/1/1/legofdm40duppo", "0x0");
			nvram_set("pci/1/1/rpcal2g", "0x0");
//			nvram_set("pci/1/1/regrev", "996");
			nvram_set("pci/1/1/rxchain", "7");
			nvram_set("pci/1/1/rxgainerr2ga0", "12");
			nvram_set("pci/1/1/rxgainerr2ga1", "-1");
			nvram_set("pci/1/1/rxgainerr2ga2", "-1");
			nvram_set("pci/1/1/sromrev", "9");
			nvram_set("pci/1/1/triso2g", "3");
			nvram_set("pci/1/1/tssipos2g", "1");
			nvram_set("pci/1/1/tempthresh", "120");
			nvram_set("pci/1/1/tempoffset", "0");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14e4");
			nvram_set("pci/1/1/watchdog", "3000");
			nvram_set("pci/1/1/xtalfreq", "20000");

			/* 5 GHz module defaults */
			nvram_set("pci/2/1/aa5g", "7");
			nvram_set("pci/2/1/aga0", "71");
			nvram_set("pci/2/1/aga1", "133");
			nvram_set("pci/2/1/aga2", "133");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x0");
			nvram_set("pci/2/1/boardvendor", "0x14e4");
//			nvram_set("pci/2/1/ccode", "Q2");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/eu_edthresh5g", "-70");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/maxp5ga0", "88,106,106,106");
			nvram_set("pci/2/1/maxp5ga1", "88,106,106,106");
			nvram_set("pci/2/1/maxp5ga2", "88,106,106,106");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0x66558655");
			nvram_set("pci/2/1/mcsbw205glpo", "0x44448888");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw405ghpo", "0x76558600");
			nvram_set("pci/2/1/mcsbw405glpo", "0x11112222");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xECA86400");
			nvram_set("pci/2/1/mcsbw805ghpo", "0x87669777");
			nvram_set("pci/2/1/mcsbw805glpo", "0x5555AAAA");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xFEA86400");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/pa5ga0", "0xff46,0x19de,0xfcdf,0xff48,0x19e9,0xfcdf,0xff4a,0x19be,0xfce6,0xff44,0x1991,0xfcea");
			nvram_set("pci/2/1/pa5ga1", "0xff44,0x19c5,0xfce4,0xff44,0x1991,0xfce9,0xff42,0x19e2,0xfcdf,0xff42,0x19f2,0xfcdb");
			nvram_set("pci/2/1/pa5ga2", "0xff48,0x19ca,0xfce9,0xff48,0x19a5,0xfceb,0xff44,0x19ea,0xfcdf,0xff46,0x19db,0xfce4");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/pwr_scale_1db", "1");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
//			nvram_set("pci/2/1/regrev", "996");
			nvram_set("pci/2/1/rpcal5gb0", "0x65c8");
			nvram_set("pci/2/1/rpcal5gb1", "0");
			nvram_set("pci/2/1/rpcal5gb2", "0");
			nvram_set("pci/2/1/rpcal5gb3", "0x76f3");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr5ga0", "-1,0,4,3");
			nvram_set("pci/2/1/rxgainerr5ga1", "-4,-5,-6,-4");
			nvram_set("pci/2/1/rxgainerr5ga2", "-2,1,-5,-7");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "1");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "1");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "1");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "2");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "2");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "2");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "2");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/watchdog", "3000");
			nvram_set("pci/2/1/venid", "0x14e4");
			nvram_set("pci/2/1/xtalfreq", "65535");
		}
		break;
	case MODEL_R6400v2:
	case MODEL_R6700v3:
	case MODEL_XR300:
		mfr = "Netgear";
		name = nvram_match("board_id", "U12H332T78_NETGEAR") ? "XR300" : nvram_match("board_id", "U12H332T77_NETGEAR") ? "R6700v3" : "R6400v2";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* disable *fake* LAN interfaces */
			nvram_unset("et1macaddr");
			nvram_unset("et2macaddr");
			nvram_unset("et3macaddr");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz defaults */
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/ag0", "0");
			nvram_set("pci/1/1/ag1", "0");
			nvram_set("pci/1/1/ag2", "0");
			nvram_set("pci/1/1/antswctl2g", "0");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags2", "0x00108000");
			nvram_set("pci/1/1/boardflags", "0x80001a00");
			nvram_set("pci/1/1/boardvendor", "0x14e4");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/devid", "0x4332");
			nvram_set("pci/1/1/elna2g", "2");
			nvram_set("pci/1/1/eu_edthresh1g", "-62");
			nvram_set("pci/1/1/extpagain2g", "0");
			nvram_set("pci/1/1/ledbh0", "11");
			nvram_set("pci/1/1/ledbh1", "11");
			nvram_set("pci/1/1/ledbh2", "11");
			nvram_set("pci/1/1/ledbh3", "11");
			nvram_set("pci/1/1/leddc", "0xFFFF");
			nvram_set("pci/1/1/legofdm40duppo", "0x0");
			nvram_set("pci/1/1/legofdmbw20ul2gpo", "0x64200000");
			nvram_set("pci/1/1/legofdmbw202gpo", "0x64200000");
			nvram_set("pci/1/1/maxp2ga0", "0x60");
			nvram_set("pci/1/1/maxp2ga1", "0x60");
			nvram_set("pci/1/1/maxp2ga2", "0x60");
			nvram_set("pci/1/1/mcs32po", "0x8");
			nvram_set("pci/1/1/mcsbw20ul2gpo", "0x86520000");
			nvram_set("pci/1/1/mcsbw202gpo", "0x86520000");
			nvram_set("pci/1/1/mcsbw402gpo", "0xEEEEEEEE");
			nvram_set("pci/1/1/pa2gw0a0", "0xfe5c");
			nvram_set("pci/1/1/pa2gw0a1", "0xfe5c");
			nvram_set("pci/1/1/pa2gw0a2", "0xfe57");
			nvram_set("pci/1/1/pa2gw1a0", "0x1cea");
			nvram_set("pci/1/1/pa2gw1a1", "0x1cea");
			nvram_set("pci/1/1/pa2gw1a2", "0x1ca9");
			nvram_set("pci/1/1/pa2gw2a0", "0xf8e5");
			nvram_set("pci/1/1/pa2gw2a1", "0xf8e6");
			nvram_set("pci/1/1/pa2gw2a2", "0xf8dc");
			nvram_set("pci/1/1/pdetrange2g", "13");
			nvram_set("pci/1/1/phycal_tempdelta", "40");
//			nvram_set("pci/1/1/regrev", "827");
			nvram_set("pci/1/1/rpcal2g", "0x0");
			nvram_set("pci/1/1/rxchain", "7");
			if (nvram_match("board_id", "U12H332T78_NETGEAR")) { /* If XR300 */
				nvram_set("pci/1/1/rxgainerr2ga0", "0x380c");
				nvram_set("pci/1/1/rxgainerr2ga1", "0x380c");
				nvram_set("pci/1/1/rxgainerr2ga2", "0x380c");
			}
			else {
				nvram_set("pci/1/1/rxgainerr2ga0", "0x4811");
				nvram_set("pci/1/1/rxgainerr2ga1", "0x4811");
				nvram_set("pci/1/1/rxgainerr2ga2", "0x4811");
			}
			nvram_set("pci/1/1/sromrev", "9");
			nvram_set("pci/1/1/tempoffset", "255");
			nvram_set("pci/1/1/temps_hysteresis", "5");
			nvram_set("pci/1/1/temps_period", "10");
			nvram_set("pci/1/1/tempthresh", "110");
			nvram_set("pci/1/1/triso2g", "3");
			nvram_set("pci/1/1/tssipos2g", "1");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14e4");
			nvram_set("pci/1/1/watchdog", "3000");
			nvram_set("pci/1/1/xtalfreq", "20000");

			/* 5 GHz module defaults */
			nvram_set("pci/2/1/aa5g", "7");
			nvram_set("pci/2/1/aga0", "71");
			nvram_set("pci/2/1/aga1", "133");
			nvram_set("pci/2/1/aga2", "133");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardvendor", "0x14e4");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/eu_edthresh5g", "-70");
			nvram_set("pci/2/1/femctrl", "6");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/maxp5ga0", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga1", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga2", "106,106,106,106");
			nvram_set("pci/2/1/mcsbw205ghpo", "0x66558600");
			nvram_set("pci/2/1/mcsbw205glpo", "0x0");
			nvram_set("pci/2/1/mcsbw205gmpo", "0x0");
			nvram_set("pci/2/1/mcsbw405ghpo", "0x76558600");
			nvram_set("pci/2/1/mcsbw405glpo", "0x0");
			nvram_set("pci/2/1/mcsbw405gmpo", "0x0");
			nvram_set("pci/2/1/mcsbw805ghpo", "0x87659000");
			nvram_set("pci/2/1/mcsbw805glpo", "0x0");
			nvram_set("pci/2/1/mcsbw805gmpo", "0x0");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/pa5ga0", "0xff46,0x19de,0xfcdc,0xff48,0x1be9,0xfcb1,0xff4a,0x1c3e,0xfcac,0xff44,0x1b91,0xfcb8");
			nvram_set("pci/2/1/pa5ga1", "0xff44,0x1945,0xfcee,0xff44,0x1b91,0xfcba,0xff42,0x1b62,0xfcbb,0xff42,0x1bf2,0xfca9");
			nvram_set("pci/2/1/pa5ga2", "0xff48,0x19ca,0xfce8,0xff48,0x1b25,0xfcc8,0xff44,0x1b6a,0xfcbb,0xff46,0x1bdb,0xfcb4");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "40");
			nvram_set("pci/2/1/pwr_scale_1db", "1");
//			nvram_set("pci/2/1/regrev", "827");
			nvram_set("pci/2/1/rxchain", "7");
			if (nvram_match("board_id", "U12H332T78_NETGEAR")) { /* If XR300 */
				nvram_set("pci/2/1/rpcal5gb0", "0x2706");
				nvram_set("pci/2/1/rpcal5gb1", "0x3201");
				nvram_set("pci/2/1/rpcal5gb2", "0x2a01");
				nvram_set("pci/2/1/rpcal5gb3", "0x380c");
				nvram_set("pci/2/1/rxgainerr5ga0", "2,0,0,1");
				nvram_set("pci/2/1/rxgainerr5ga1", "-1,0,0,-8");
			}
			else {
				nvram_set("pci/2/1/rpcal5gb0", "0x4e17");
				nvram_set("pci/2/1/rpcal5gb1", "0x5113");
				nvram_set("pci/2/1/rpcal5gb2", "0x3c0b");
				nvram_set("pci/2/1/rpcal5gb3", "0x4811");
				nvram_set("pci/2/1/rxgainerr5ga0", "4,0,0,5");
				nvram_set("pci/2/1/rxgainerr5ga1", "-5,0,0,-4");
			}
			nvram_set("pci/2/1/rxgainerr5ga2", "1,0,0,-2");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "5");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "5");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "5");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/temps_hysteresis", "5");
			nvram_set("pci/2/1/temps_period", "10");
			nvram_set("pci/2/1/tempthresh", "110");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/venid", "0x14e4");
			nvram_set("pci/2/1/watchdog", "3000");
			nvram_set("pci/2/1/xtalfreq", "65535");
		}
		break;
	case MODEL_R6700v1:
	case MODEL_R6900:
	case MODEL_R7000:
		mfr = "Netgear";
		name = nvram_match("board_id", "U12H270T00_NETGEAR") ? "R7000" : nvram_match("board_id", "U12H270T11_NETGEAR") ? "R6900" : "R6700v1";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* disable second *fake* LAN interface */
			nvram_unset("et1macaddr");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			/* 2.4 GHz defaults */
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/agbg0", "0");
			nvram_set("pci/1/1/agbg1", "0");
			nvram_set("pci/1/1/agbg2", "0");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags", "0x1000");
			nvram_set("pci/1/1/boardflags2", "0x100002");
			nvram_set("pci/1/1/boardflags3", "0x10000003");
			nvram_set("pci/1/1/boardnum", "57359");
			nvram_set("pci/1/1/boardrev", "0x1150");
			nvram_set("pci/1/1/boardtype", "0x661");
			nvram_set("pci/1/1/boardvendor", "0x14E4");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/devid", "0x43a1");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/dot11agofdmhrbw202gpo", "0xCA86");
			nvram_set("pci/1/1/epagain2g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
			nvram_set("pci/1/1/maxp2ga0", "106");
			nvram_set("pci/1/1/maxp2ga1", "106");
			nvram_set("pci/1/1/maxp2ga2", "106");
			nvram_set("pci/1/1/mcsbw202gpo", "0xA976A600");
			nvram_set("pci/1/1/mcsbw402gpo", "0xA976A600");
			nvram_set("pci/1/1/measpower", "0x7f");
			nvram_set("pci/1/1/measpower1", "0x7f");
			nvram_set("pci/1/1/measpower2", "0x7f");
			nvram_set("pci/1/1/noiselvl2ga0", "31");
			nvram_set("pci/1/1/noiselvl2ga1", "31");
			nvram_set("pci/1/1/noiselvl2ga2", "31");
			nvram_set("pci/1/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/1/1/pa2ga0", "0xFF32,0x1C30,0xFCA3");
			nvram_set("pci/1/1/pa2ga1", "0xFF35,0x1BE3,0xFCB0");
			nvram_set("pci/1/1/pa2ga2", "0xFF33,0x1BE1,0xFCB0");
			nvram_set("pci/1/1/papdcap2g", "0");
			nvram_set("pci/1/1/pdgain2g", "14");
			nvram_set("pci/1/1/pdoffset2g40ma0", "15");
			nvram_set("pci/1/1/pdoffset2g40ma1", "15");
			nvram_set("pci/1/1/pdoffset2g40ma2", "15");
			nvram_set("pci/1/1/pdoffset2g40mvalid", "1");
			nvram_set("pci/1/1/pdoffset40ma0", "0");
			nvram_set("pci/1/1/pdoffset40ma1", "0");
			nvram_set("pci/1/1/pdoffset40ma2", "0");
			nvram_set("pci/1/1/pdoffset80ma0", "0");
			nvram_set("pci/1/1/pdoffset80ma1", "0");
			nvram_set("pci/1/1/pdoffset80ma2", "0");
//			nvram_set("pci/1/1/regrev", "53");
			nvram_set("pci/1/1/rpcal2g", "0x3ef");
			nvram_set("pci/1/1/rxchain", "7");
			nvram_set("pci/1/1/rxgainerr2ga0", "63");
			nvram_set("pci/1/1/rxgainerr2ga1", "31");
			nvram_set("pci/1/1/rxgainerr2ga2", "31");
			nvram_set("pci/1/1/rxgains2gelnagaina0", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina1", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina2", "3");
			nvram_set("pci/1/1/rxgains2gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains2gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa1", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa2", "7");
			nvram_set("pci/1/1/sar2g", "18");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "0x4");
			nvram_set("pci/1/1/subvid", "0x14e4");
			nvram_set("pci/1/1/tssifloor2g", "0x3ff");
			nvram_set("pci/1/1/temps_period", "5");
			nvram_set("pci/1/1/tempthresh", "120");
			nvram_set("pci/1/1/tempoffset", "0");
			nvram_set("pci/1/1/tssiposslope2g", "1");
			nvram_set("pci/1/1/tworangetssi2g", "0");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14e4");
			nvram_set("pci/1/1/xtalfreq", "65535");

			/* 5 GHz module defaults */
			nvram_set("pci/2/1/aa5g", "0");
			nvram_set("pci/2/1/aga0", "0");
			nvram_set("pci/2/1/aga1", "0");
			nvram_set("pci/2/1/aga2", "0");
			nvram_set("pci/2/1/agbg0", "0");
			nvram_set("pci/2/1/agbg1", "0");
			nvram_set("pci/2/1/agbg2", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x10000000");
			nvram_set("pci/2/1/boardnum", "20507");
			nvram_set("pci/2/1/boardrev", "0x1451");
			nvram_set("pci/2/1/boardtype", "0x621");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/maxp5ga0", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga1", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga2", "106,106,106,106");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/pa5ga0", "0xFF4C,0x1808,0xFD1B,0xFF4C,0x18CF,0xFD0C,0xFF4A,0x1920,0xFD08,0xFF4C,0x1949,0xFCF6");
			nvram_set("pci/2/1/pa5ga1", "0xFF4A,0x18AC,0xFD0B,0xFF44,0x1904,0xFCFF,0xFF56,0x1A09,0xFCFC,0xFF4F,0x19AB,0xFCEF");
			nvram_set("pci/2/1/pa5ga2", "0xFF4C,0x1896,0xFD11,0xFF43,0x192D,0xFCF5,0xFF50,0x19EE,0xFCF1,0xFF52,0x19C6,0xFCF1");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
//			nvram_set("pci/2/1/regrev", "53");
			nvram_set("pci/2/1/rpcal5gb0", "0x7005");
			nvram_set("pci/2/1/rpcal5gb1", "0x8403");
			nvram_set("pci/2/1/rpcal5gb2", "0x6ff9");
			nvram_set("pci/2/1/rpcal5gb3", "0x8509");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
			nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
			nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/subvid", "0x14e4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/tssifloor5g", "0x3ff,0x3ff,0x3ff,0x3ff");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/xtalfreq", "65535");
		}
		break;
	case MODEL_DIR868L:
		mfr = "D-Link";
		name = "DIR868L";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* Case DIR868L rev C1 */
			if (nvram_match("boardrev", "0x1101")) {

				/* fix MAC addresses */
				strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
				inc_mac(s, +2);					/* MAC + 1 will be for WAN */
				nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
				nvram_set("wl0_hwaddr", s);
				inc_mac(s, +4);					/* do not overlap with VIFs */
				nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
				nvram_set("wl1_hwaddr", s);

				/* wifi country settings */
				nvram_set("0:regrev", "12");
				nvram_set("1:regrev", "12");
				nvram_set("0:ccode", "SG");
				nvram_set("1:ccode", "SG");
			}
			else { /* Case DIR868L rev A1/B1 */

				/* fix MAC addresses */
				strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
				inc_mac(s, +2);					/* MAC + 1 will be for WAN */
				nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 2,4G */
				nvram_set("wl0_hwaddr", s);
				inc_mac(s, +4);					/* do not overlap with VIFs */
				nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 5G */
				nvram_set("wl1_hwaddr", s);

				/* wifi country settings */
				nvram_set("pci/1/1/regrev", "12");
				nvram_set("pci/2/1/regrev", "12");
				nvram_set("pci/1/1/ccode", "SG");
				nvram_set("pci/2/1/ccode", "SG");

				/* enable 5 GHz WLAN for rev A1/B1 */
				nvram_unset("devpath1");
			}

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */
			
			/* Case DIR868L rev C1 */
			if (nvram_match("boardrev", "0x1101")) {

				/* 2.4 GHz defaults */
				nvram_set("devpath0", "pci/1/1/");
				nvram_set("0:aa2g", "7");
				nvram_set("0:ag0", "0");
				nvram_set("0:ag1", "0");
				nvram_set("0:ag2", "0");
				nvram_set("0:antswitch", "0");
				nvram_set("0:antswctl2g", "0");
				nvram_set("0:boardflags", "0x80001200");
				nvram_set("0:boardflags2", "0x00100000");
				nvram_set("0:boardvendor", "0x14E4");
//				nvram_set("0:ccode", "SG");
				nvram_set("0:cckbw20ul2gpo", "0x2200");
				nvram_set("0:cckbw202gpo", "0x2200");
				nvram_set("0:devid", "0x4332");
				nvram_set("0:elna2g", "2");
				nvram_set("0:extpagain2g", "3");
				nvram_set("0:maxp2ga0", "0x74");
				nvram_set("0:maxp2ga1", "0x74");
				nvram_set("0:maxp2ga2", "0x74");
				nvram_set("0:mcsbw20ul2gpo", "0x88765433");
				nvram_set("0:mcsbw202gpo", "0x88765433");
				nvram_set("0:mcsbw402gpo", "0x99855433");
				nvram_set("0:mcs32po", "0x0003");
				nvram_set("0:pa2gw0a0", "0xfea3");
				nvram_set("0:pa2gw0a1", "0xfed1");
				nvram_set("0:pa2gw0a2", "0xfe6d");
				nvram_set("0:pa2gw1a0", "0x1577");
				nvram_set("0:pa2gw1a1", "0x15d2");
				nvram_set("0:pa2gw1a2", "0x1489");
				nvram_set("0:pa2gw2a0", "0xfadb");
				nvram_set("0:pa2gw2a1", "0xfb15");
				nvram_set("0:pa2gw2a2", "0xfac2");
				nvram_set("0:parefldovoltage", "60");
				nvram_set("0:pdetrange2g", "4");
				nvram_set("0:phycal_tempdelta", "0");
				nvram_set("0:ledbh0", "11");
				nvram_set("0:ledbh1", "11");
				nvram_set("0:ledbh2", "11");
				nvram_set("0:ledbh3", "11");
				nvram_set("0:leddc", "0xFFFF");
				nvram_set("0:legofdmbw202gpo", "0x88765433");
				nvram_set("0:legofdmbw20ul2gpo", "0x88765433");
				nvram_set("0:legofdm40duppo", "0x0000");
//				nvram_set("0:regrev", "0");
				nvram_set("0:rxchain", "7");
				nvram_set("0:sromrev", "9");
				nvram_set("0:tssipos2g", "1");
				nvram_set("0:tempthresh", "120");
				nvram_set("0:tempoffset", "0");
				nvram_set("0:temps_period", "5");
				nvram_set("0:temps_hysteresis", "5");
				nvram_set("0:txchain", "7");
				nvram_set("0:venid", "0x14E4");
				nvram_set("0:xtalfreq", "20000");

				/* 5 GHz module defaults */
				nvram_set("devpath1", "pci/2/1");
				nvram_set("1:aa5g", "7");
				nvram_set("1:aga0", "0");
				nvram_set("1:aga1", "0");
				nvram_set("1:aga2", "0");
				nvram_set("1:antswitch", "0");
				nvram_set("1:boardflags", "0x10000000");
				nvram_set("1:boardflags2", "0x00000002");
				nvram_set("1:boardflags3", "0x00000000");
//				nvram_set("1:ccode", "SG");
				nvram_set("1:devid", "0x43a2");
				nvram_set("1:dot11agduphrpo", "0");
				nvram_set("1:dot11agduplrpo", "0");
				nvram_set("1:epagain5g", "0");
				nvram_set("1:femctrl", "6");
				nvram_set("1:gainctrlsph", "0");
				nvram_set("1:ledbh0", "11");
				nvram_set("1:ledbh1", "11");
				nvram_set("1:ledbh2", "11");
				nvram_set("1:ledbh3", "11");
				nvram_set("1:leddc", "0xFFFF");
				nvram_set("1:maxp5ga0", "0x56,0x56,0x56,0x56");
				nvram_set("1:maxp5ga1", "0x56,0x56,0x56,0x56");
				nvram_set("1:maxp5ga2", "0x56,0x56,0x56,0x56");
				nvram_set("1:mcsbw1605ghpo", "0");
				nvram_set("1:mcsbw1605glpo", "0");
				nvram_set("1:mcsbw1605gmpo", "0");
				nvram_set("1:mcsbw205ghpo", "0x87420000");
				nvram_set("1:mcsbw205glpo", "0x87420000");
				nvram_set("1:mcsbw205gmpo", "0x87420000");
				nvram_set("1:mcsbw405ghpo", "0x87420000");
				nvram_set("1:mcsbw405glpo", "0x87420000");
				nvram_set("1:mcsbw405gmpo", "0x87420000");
				nvram_set("1:mcsbw805ghpo", "0x87420000");
				nvram_set("1:mcsbw805glpo", "0x87420000");
				nvram_set("1:mcsbw805gmpo", "0x87420000");
				nvram_set("1:mcslr5ghpo", "0");
				nvram_set("1:mcslr5glpo", "0");
				nvram_set("1:mcslr5gmpo", "0");
				nvram_set("1:measpower", "0x7f");
				nvram_set("1:measpower1", "0x7f");
				nvram_set("1:measpower2", "0x7f");
				nvram_set("1:noiselvl5ga0", "31,31,31,31");
				nvram_set("1:noiselvl5ga1", "31,31,31,31");
				nvram_set("1:noiselvl5ga2", "31,31,31,31");
				nvram_set("1:pa5ga0", "0xff2a,0x173b,0xfd0e,0xff32,0x17b6,0xfd07,0xff27,0x163a,0xfd1c,0xff39,0x1696,0xfd1b");
				nvram_set("1:pa5ga1", "0xff1c,0x1680,0xfd0e,0xff44,0x18b6,0xfcf2,0xff25,0x1593,0xfd20,0xff4a,0x1694,0xfd1c");
				nvram_set("1:pa5ga2", "0xff44,0x1836,0xfd07,0xff47,0x1844,0xfd04,0xff33,0x167b,0xfd17,0xff4f,0x1690,0xfd33");
				nvram_set("1:papdcap5g", "0");
				nvram_set("1:pdgain5g", "19");
				nvram_set("1:pdoffset40ma0", "0x5444");
				nvram_set("1:pdoffset40ma1", "0x5444");
				nvram_set("1:pdoffset40ma2", "0x5344");
				nvram_set("1:pdoffset80ma0", "0x2111");
				nvram_set("1:pdoffset80ma1", "0x0111");
				nvram_set("1:pdoffset80ma2", "0x2111");
				nvram_set("1:phycal_tempdelta", "0");
				nvram_set("1:rawtempsense", "0x1ff");
//				nvram_set("1:regrev", "0");
				nvram_set("1:rxchain", "7");
				nvram_set("1:rxgainerr5ga0", "63,63,63,63");
				nvram_set("1:rxgainerr5ga1", "31,31,31,31");
				nvram_set("1:rxgainerr5ga2", "31,31,31,31");
				nvram_set("1:rxgains5gelnagaina0", "3");
				nvram_set("1:rxgains5gelnagaina1", "3");
				nvram_set("1:rxgains5gelnagaina2", "3");
				nvram_set("1:rxgains5ghelnagaina0", "3");
				nvram_set("1:rxgains5ghelnagaina1", "3");
				nvram_set("1:rxgains5ghelnagaina2", "3");
				nvram_set("1:rxgains5ghtrelnabypa0", "1");
				nvram_set("1:rxgains5ghtrelnabypa1", "1");
				nvram_set("1:rxgains5ghtrelnabypa2", "1");
				nvram_set("1:rxgains5ghtrisoa0", "6");
				nvram_set("1:rxgains5ghtrisoa1", "6");
				nvram_set("1:rxgains5ghtrisoa2", "6");
				nvram_set("1:rxgains5gmelnagaina0", "3");
				nvram_set("1:rxgains5gmelnagaina1", "3");
				nvram_set("1:rxgains5gmelnagaina2", "3");
				nvram_set("1:rxgains5gmtrelnabypa0", "1");
				nvram_set("1:rxgains5gmtrelnabypa1", "1");
				nvram_set("1:rxgains5gmtrelnabypa2", "1");
				nvram_set("1:rxgains5gmtrisoa0", "6");
				nvram_set("1:rxgains5gmtrisoa1", "6");
				nvram_set("1:rxgains5gmtrisoa2", "6");
				nvram_set("1:rxgains5gtrelnabypa0", "1");
				nvram_set("1:rxgains5gtrelnabypa1", "1");
				nvram_set("1:rxgains5gtrelnabypa2", "1");
				nvram_set("1:rxgains5gtrisoa0", "6");
				nvram_set("1:rxgains5gtrisoa1", "6");
				nvram_set("1:rxgains5gtrisoa2", "6");
				nvram_set("1:sar5g", "15");
				nvram_set("1:sb20in40hrpo", "0");
				nvram_set("1:sb20in40lrpo", "0");
				nvram_set("1:sb20in80and160hr5ghpo", "0");
				nvram_set("1:sb20in80and160hr5glpo", "0");
				nvram_set("1:sb20in80and160hr5gmpo", "0");
				nvram_set("1:sb20in80and160lr5ghpo", "0");
				nvram_set("1:sb20in80and160lr5glpo", "0");
				nvram_set("1:sb20in80and160lr5gmpo", "0");
				nvram_set("1:sb40and80hr5ghpo", "0");
				nvram_set("1:sb40and80hr5glpo", "0");
				nvram_set("1:sb40and80hr5gmpo", "0");
				nvram_set("1:sb40and80lr5ghpo", "0");
				nvram_set("1:sb40and80lr5glpo", "0");
				nvram_set("1:sb40and80lr5gmpo", "0");
				nvram_set("1:sromrev", "11");
				nvram_set("1:subband5gver", "4");
				nvram_set("1:tempcorrx", "0x3f");
				nvram_set("1:tempoffset", "0");
				nvram_set("1:tempsense_option", "0x3");
				nvram_set("1:tempsense_slope", "0xff");
				nvram_set("1:temps_hysteresis", "15");
				nvram_set("1:temps_period", "15");
				nvram_set("1:tempthresh", "120");
				nvram_set("1:tssiposslope5g", "1");
				nvram_set("1:tworangetssi5g", "0");
				nvram_set("1:txchain", "7");
				nvram_set("1:venid", "0x14e4");
				nvram_set("1:xtalfreq", "40000");
			}
			else { /* Case DIR868L rev A1/B1 */

				/* bcm4360ac_defaults - fix problem of loading driver failed with code 21 */
				nvram_set("pci/1/1/aa2g", "7");
				nvram_set("pci/1/1/agbg0", "71");
				nvram_set("pci/1/1/agbg1", "71");
				nvram_set("pci/1/1/agbg2", "71");
				nvram_set("pci/1/1/antswitch", "0");
				nvram_set("pci/1/1/boardflags", "0x1000");
				nvram_set("pci/1/1/boardflags2", "0x100002");
				nvram_set("pci/1/1/boardflags3", "0x10000003");
				nvram_set("pci/1/1/boardnum", "57359");
				nvram_set("pci/1/1/boardrev", "0x1150");
				nvram_set("pci/1/1/boardtype", "0x661");
				nvram_set("pci/1/1/cckbw202gpo", "0");
				nvram_set("pci/1/1/cckbw20ul2gpo", "0");
//				nvram_set("pci/1/1/ccode", "SG");
				nvram_set("pci/1/1/devid", "0x43a1");
				nvram_set("pci/1/1/dot11agduphrpo", "0");
				nvram_set("pci/1/1/dot11agduplrpo", "0");
				nvram_set("pci/1/1/dot11agofdmhrbw202gpo", "0xCA86");
				nvram_set("pci/1/1/epagain2g", "0");
				nvram_set("pci/1/1/femctrl", "3");
				nvram_set("pci/1/1/gainctrlsph", "0");
//				nvram_set("pci/1/1/macaddr", "E4:F4:C6:01:47:7C");
				nvram_set("pci/1/1/maxp2ga0", "106");
				nvram_set("pci/1/1/maxp2ga1", "106");
				nvram_set("pci/1/1/maxp2ga2", "106");
				nvram_set("pci/1/1/mcsbw202gpo", "0xA976A600");
				nvram_set("pci/1/1/mcsbw402gpo", "0xA976A600");
				nvram_set("pci/1/1/measpower", "0x7f");
				nvram_set("pci/1/1/measpower1", "0x7f");
				nvram_set("pci/1/1/measpower2", "0x7f");
				nvram_set("pci/1/1/noiselvl2ga0", "31");
				nvram_set("pci/1/1/noiselvl2ga1", "31");
				nvram_set("pci/1/1/noiselvl2ga2", "31");
				nvram_set("pci/1/1/ofdmlrbw202gpo", "0");
				nvram_set("pci/1/1/pa2ga0", "0xFF32,0x1C30,0xFCA3");
				nvram_set("pci/1/1/pa2ga1", "0xFF35,0x1BE3,0xFCB0");
				nvram_set("pci/1/1/pa2ga2", "0xFF33,0x1BE1,0xFCB0");
				nvram_set("pci/1/1/papdcap2g", "0");
				nvram_set("pci/1/1/pdgain2g", "14");
				nvram_set("pci/1/1/pdoffset2g40ma0", "15");
				nvram_set("pci/1/1/pdoffset2g40ma1", "15");
				nvram_set("pci/1/1/pdoffset2g40ma2", "15");
				nvram_set("pci/1/1/pdoffset2g40mvalid", "1");
				nvram_set("pci/1/1/pdoffset40ma0", "0");
				nvram_set("pci/1/1/pdoffset40ma1", "0");
				nvram_set("pci/1/1/pdoffset40ma2", "0");
				nvram_set("pci/1/1/pdoffset80ma0", "0");
				nvram_set("pci/1/1/pdoffset80ma1", "0");
				nvram_set("pci/1/1/pdoffset80ma2", "0");
//				nvram_set("pci/1/1/regrev", "66");
				nvram_set("pci/1/1/rpcal2g", "0x5f7");
				nvram_set("pci/1/1/rxgainerr2ga0", "63");
				nvram_set("pci/1/1/rxgainerr2ga1", "31");
				nvram_set("pci/1/1/rxgainerr2ga2", "31");
				nvram_set("pci/1/1/rxgains2gelnagaina0", "3");
				nvram_set("pci/1/1/rxgains2gelnagaina1", "3");
				nvram_set("pci/1/1/rxgains2gelnagaina2", "3");
				nvram_set("pci/1/1/rxgains2gtrelnabypa0", "1");
				nvram_set("pci/1/1/rxgains2gtrelnabypa1", "1");
				nvram_set("pci/1/1/rxgains2gtrelnabypa2", "1");
				nvram_set("pci/1/1/rxgains2gtrisoa0", "7");
				nvram_set("pci/1/1/rxgains2gtrisoa1", "7");
				nvram_set("pci/1/1/rxgains2gtrisoa2", "7");
				nvram_set("pci/1/1/sar2g", "18");
				nvram_set("pci/1/1/sromrev", "11");
				nvram_set("pci/1/1/subband5gver", "0x4");
				nvram_set("pci/1/1/subvid", "0x14e4");
				nvram_set("pci/1/1/tssifloor2g", "0x3ff");
				nvram_set("pci/1/1/tssiposslope2g", "1");
				nvram_set("pci/1/1/tworangetssi2g", "0");
				nvram_set("pci/1/1/xtalfreq", "65535");
				nvram_set("pci/2/1/aa2g", "7");
				nvram_set("pci/2/1/aa5g", "0");
				nvram_set("pci/2/1/aga0", "0");
				nvram_set("pci/2/1/aga1", "0");
				nvram_set("pci/2/1/aga2", "0");
				nvram_set("pci/2/1/agbg0", "0");
				nvram_set("pci/2/1/agbg1", "0");
				nvram_set("pci/2/1/agbg2", "0");
				nvram_set("pci/2/1/antswitch", "0");
				nvram_set("pci/2/1/boardflags", "0x30000000");
				nvram_set("pci/2/1/boardflags2", "0x300002");
				nvram_set("pci/2/1/boardflags3", "0x10000000");
				nvram_set("pci/2/1/boardnum", "20507");
				nvram_set("pci/2/1/boardrev", "0x1451");
				nvram_set("pci/2/1/boardtype", "0x621");
				nvram_set("pci/2/1/cckbw202gpo", "0");
				nvram_set("pci/2/1/cckbw20ul2gpo", "0");
//				nvram_set("pci/2/1/ccode", "SG");
				nvram_set("pci/2/1/devid", "0x43a2");
				nvram_set("pci/2/1/dot11agduphrpo", "0");
				nvram_set("pci/2/1/dot11agduplrpo", "0");
				nvram_set("pci/2/1/dot11agofdmhrbw202gpo", "0");
				nvram_set("pci/2/1/epagain2g", "0");
				nvram_set("pci/2/1/epagain5g", "0");
				nvram_set("pci/2/1/femctrl", "3");
				nvram_set("pci/2/1/gainctrlsph", "0");
//				nvram_set("pci/2/1/macaddr", "E4:F4:C6:01:47:7B");
				nvram_set("pci/2/1/maxp2ga0", "76");
				nvram_set("pci/2/1/maxp2ga1", "76");
				nvram_set("pci/2/1/maxp2ga2", "76");
				nvram_set("pci/2/1/maxp5ga0", "106,106,106,106");
				nvram_set("pci/2/1/maxp5ga1", "106,106,106,106");
				nvram_set("pci/2/1/maxp5ga2", "106,106,106,106");
				nvram_set("pci/2/1/mcsbw1605ghpo", "0");
				nvram_set("pci/2/1/mcsbw1605glpo", "0");
				nvram_set("pci/2/1/mcsbw1605gmpo", "0");
				nvram_set("pci/2/1/mcsbw1605hpo", "0");
				nvram_set("pci/2/1/mcsbw202gpo", "0");
				nvram_set("pci/2/1/mcsbw205ghpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw205glpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw205gmpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw402gpo", "0");
				nvram_set("pci/2/1/mcsbw405ghpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw405glpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw405gmpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw805ghpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw805glpo", "0xBA768600");
				nvram_set("pci/2/1/mcsbw805gmpo", "0xBA768600");
				nvram_set("pci/2/1/mcslr5ghpo", "0");
				nvram_set("pci/2/1/mcslr5glpo", "0");
				nvram_set("pci/2/1/mcslr5gmpo", "0");
				nvram_set("pci/2/1/measpower", "0x7f");
				nvram_set("pci/2/1/measpower1", "0x7f");
				nvram_set("pci/2/1/measpower2", "0x7f");
				nvram_set("pci/2/1/noiselvl2ga0", "31");
				nvram_set("pci/2/1/noiselvl2ga1", "31");
				nvram_set("pci/2/1/noiselvl2ga2", "31");
				nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
				nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
				nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
				nvram_set("pci/2/1/ofdmlrbw202gpo", "0");
				nvram_set("pci/2/1/pa2ga0", "0xfe72,0x14c0,0xfac7");
				nvram_set("pci/2/1/pa2ga1", "0xfe80,0x1472,0xfabc");
				nvram_set("pci/2/1/pa2ga2", "0xfe82,0x14bf,0xfad9");
				nvram_set("pci/2/1/pa5ga0", "0xFF4C,0x1808,0xFD1B,0xFF4C,0x18CF,0xFD0C,0xFF4A,0x1920,0xFD08,0xFF4C,0x1949,0xFCF6");
				nvram_set("pci/2/1/pa5ga1", "0xFF4A,0x18AC,0xFD0B,0xFF44,0x1904,0xFCFF,0xFF56,0x1A09,0xFCFC,0xFF4F,0x19AB,0xFCEF");
				nvram_set("pci/2/1/pa5ga2", "0xFF4C,0x1896,0xFD11,0xFF43,0x192D,0xFCF5,0xFF50,0x19EE,0xFCF1,0xFF52,0x19C6,0xFCF1");
				nvram_set("pci/2/1/papdcap2g", "0");
				nvram_set("pci/2/1/papdcap5g", "0");
				nvram_set("pci/2/1/pdgain2g", "4");
				nvram_set("pci/2/1/pdgain5g", "4");
				nvram_set("pci/2/1/pdoffset2g40ma0", "15");
				nvram_set("pci/2/1/pdoffset2g40ma1", "15");
				nvram_set("pci/2/1/pdoffset2g40ma2", "15");
				nvram_set("pci/2/1/pdoffset2g40mvalid", "1");
				nvram_set("pci/2/1/pdoffset40ma0", "4369");
				nvram_set("pci/2/1/pdoffset40ma1", "4369");
				nvram_set("pci/2/1/pdoffset40ma2", "4369");
				nvram_set("pci/2/1/pdoffset80ma0", "0");
				nvram_set("pci/2/1/pdoffset80ma1", "0");
				nvram_set("pci/2/1/pdoffset80ma2", "0");
				nvram_set("pci/2/1/phycal_tempdelta", "255");
				nvram_set("pci/2/1/rawtempsense", "0x1ff");
//				nvram_set("pci/2/1/regrev", "66");
				nvram_set("pci/2/1/rpcal2g", "0");
				nvram_set("pci/2/1/rpcal5gb0", "0x610c");
				nvram_set("pci/2/1/rpcal5gb1", "0x6a09");
				nvram_set("pci/2/1/rpcal5gb2", "0x5eff");
				nvram_set("pci/2/1/rpcal5gb3", "0x700c");
				nvram_set("pci/2/1/rxchain", "7");
				nvram_set("pci/2/1/rxgainerr2ga0", "63");
				nvram_set("pci/2/1/rxgainerr2ga1", "31");
				nvram_set("pci/2/1/rxgainerr2ga2", "31");
				nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
				nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
				nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
				nvram_set("pci/2/1/rxgains2gelnagaina0", "0");
				nvram_set("pci/2/1/rxgains2gelnagaina1", "0");
				nvram_set("pci/2/1/rxgains2gelnagaina2", "0");
				nvram_set("pci/2/1/rxgains2gtrelnabypa0", "0");
				nvram_set("pci/2/1/rxgains2gtrelnabypa1", "0");
				nvram_set("pci/2/1/rxgains2gtrelnabypa2", "0");
				nvram_set("pci/2/1/rxgains2gtrisoa0", "0");
				nvram_set("pci/2/1/rxgains2gtrisoa1", "0");
				nvram_set("pci/2/1/rxgains2gtrisoa2", "0");
				nvram_set("pci/2/1/rxgains5gelnagaina0", "4");
				nvram_set("pci/2/1/rxgains5gelnagaina1", "4");
				nvram_set("pci/2/1/rxgains5gelnagaina2", "4");
				nvram_set("pci/2/1/rxgains5ghelnagaina0", "3");
				nvram_set("pci/2/1/rxgains5ghelnagaina1", "3");
				nvram_set("pci/2/1/rxgains5ghelnagaina2", "4");
				nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
				nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
				nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
				nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
				nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
				nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
				nvram_set("pci/2/1/rxgains5gmelnagaina0", "3");
				nvram_set("pci/2/1/rxgains5gmelnagaina1", "4");
				nvram_set("pci/2/1/rxgains5gmelnagaina2", "4");
				nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
				nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
				nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
				nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
				nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
				nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
				nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
				nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
				nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
				nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
				nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
				nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
				nvram_set("pci/2/1/sar2g", "18");
				nvram_set("pci/2/1/sar5g", "15");
				nvram_set("pci/2/1/sb20in40hrpo", "0");
				nvram_set("pci/2/1/sb20in40lrpo", "0");
				nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
				nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
				nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
				nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
				nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
				nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
				nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
				nvram_set("pci/2/1/sb40and80hr5glpo", "0");
				nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
				nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
				nvram_set("pci/2/1/sb40and80lr5glpo", "0");
				nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
				nvram_set("pci/2/1/sromrev", "11");
				nvram_set("pci/2/1/subband5gver", "0x4");
				nvram_set("pci/2/1/subvid", "0x14e4");
				nvram_set("pci/2/1/tempcorrx", "0x3f");
				nvram_set("pci/2/1/tempoffset", "255");
				nvram_set("pci/2/1/tempsense_option", "0x3");
				nvram_set("pci/2/1/tempsense_slope", "0xff");
				nvram_set("pci/2/1/temps_hysteresis", "15");
				nvram_set("pci/2/1/temps_period", "15");
				nvram_set("pci/2/1/tempthresh", "255");
				nvram_set("pci/2/1/tssifloor2g", "0x3ff");
				nvram_set("pci/2/1/tssifloor5g", "0x3ff,0x3ff,0x3ff,0x3ff");
				nvram_set("pci/2/1/tssiposslope2g", "1");
				nvram_set("pci/2/1/tssiposslope5g", "1");
				nvram_set("pci/2/1/tworangetssi2g", "0");
				nvram_set("pci/2/1/tworangetssi5g", "0");
				nvram_set("pci/2/1/txchain", "7");
				nvram_set("pci/2/1/xtalfreq", "65535");
			} /* Case DIR868L rev A1/B1 */
		}
		break;
	case MODEL_WS880:
		mfr = "Huawei";
		name = "WS880";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");
			nvram_set("blink_wl", "0");			/* disable blink by default for WS880 */

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* 2.4 GHz module defaults */
			nvram_set("devpath0", "pci/1/1");
			nvram_set("0:aa2g", "7");
			nvram_set("0:ag0", "0");
			nvram_set("0:ag1", "0");
			nvram_set("0:ag2", "0");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags2", "0x00100000");
			nvram_set("0:boardflags", "0x80001200");
			nvram_set("0:boardtype", "0x59b");
			nvram_set("0:boardvendor", "0x14e4");
			nvram_set("0:cckbw202gpo", "0x0000");
			nvram_set("0:cckbw20ul2gpo", "0x0000");
//			nvram_set("0:ccode", "#a");
			nvram_set("0:devid", "0x4332");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:ledbh0", "11");
//			nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "14");
			nvram_set("0:ledbh3", "1");
			nvram_set("0:ledbh12", "11");
			nvram_set("0:leddc", "0xffff");
			nvram_set("0:legofdm40duppo", "0x0");
			nvram_set("0:legofdmbw202gpo", "0x88888888");
			nvram_set("0:legofdmbw20ul2gpo", "0x88888888");
			nvram_set("0:maxp2ga0", "0x46");
			nvram_set("0:maxp2ga1", "0x46");
			nvram_set("0:maxp2ga2", "0x46");
			nvram_set("0:mcs32po", "0x0");
			nvram_set("0:mcsbw202gpo", "0x88888888");
			nvram_set("0:mcsbw20ul2gpo", "0x88888888");
			nvram_set("0:mcsbw402gpo", "0x88888888");
			nvram_set("0:pa2gw0a0", "0xfe63");
			nvram_set("0:pa2gw0a1", "0xfe78");
			nvram_set("0:pa2gw0a2", "0xfe65");
			nvram_set("0:pa2gw1a0", "0x1dfd");
			nvram_set("0:pa2gw1a1", "0x1e4a");
			nvram_set("0:pa2gw1a2", "0x1e74");
			nvram_set("0:pa2gw2a0", "0xf8c7");
			nvram_set("0:pa2gw2a1", "0xf8d8");
			nvram_set("0:pa2gw2a2", "0xf8b9");
			nvram_set("0:parefldovoltage", "35");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
//			nvram_set("0:regrev", "0");
			nvram_set("0:rxchain", "7");
			nvram_set("0:sromrev", "9");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:triso2g", "3");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14e4");
			nvram_set("0:xtalfreq", "20000");

			/* 5 GHz module defaults */
			nvram_set("devpath1", "pci/2/1");
			nvram_set("1:aa2g", "0");
			nvram_set("1:aa5g", "7");
			nvram_set("1:agbg0", "71");
			nvram_set("1:agbg1", "71");
			nvram_set("1:agbg2", "133");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags2", "0x00300002");
			nvram_set("1:boardflags3", "0x0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardnum", "20771");
			nvram_set("1:boardrev", "0x1402");
			nvram_set("1:boardtype", "0x621");
			nvram_set("1:boardvendor", "0x14e4");
			nvram_set("1:cckbw202gpo", "0");
			nvram_set("1:cckbw20ul2gpo", "0");
//			nvram_set("1:ccode", "#a");
			nvram_set("1:devid", "0x43a2");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain2g", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:maxp2ga0", "76");
			nvram_set("1:maxp2ga1", "76");
			nvram_set("1:maxp2ga2", "76");
			nvram_set("1:maxp5ga0", "70,70,86,86");
			nvram_set("1:maxp5ga1", "70,70,86,86");
			nvram_set("1:maxp5ga2", "70,70,86,86");
			nvram_set("1:mcsbw402gpo", "0");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw402gpo", "0");
			nvram_set("1:mcsbw1605glpo", "0x00222222");
			nvram_set("1:mcsbw205ghpo", "0xaa880000");
			nvram_set("1:mcsbw205glpo", "0x66666666");
			nvram_set("1:mcsbw205gmpo", "0x66666666");
			nvram_set("1:mcsbw405ghpo", "0xaa880000");
			nvram_set("1:mcsbw405glpo", "0x22222222"); 
			nvram_set("1:mcsbw405gmpo", "0x22222222");
			nvram_set("1:mcsbw805ghpo", "0x88880000");
			nvram_set("1:mcsbw805glpo", "0x00222222");
			nvram_set("1:mcsbw805gmpo", "0x00222222");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:measpower1", "0x7f");
			nvram_set("1:measpower2", "0x7f");
			nvram_set("1:measpower", "0x7f");
			nvram_set("1:noiselvl2ga0", "31");
			nvram_set("1:noiselvl2ga1", "31");
			nvram_set("1:noiselvl2ga2", "31");
			nvram_set("1:noiselvl5ga0", "31,31,31,31");
			nvram_set("1:noiselvl5ga1", "31,31,31,31");
			nvram_set("1:noiselvl5ga2", "31,31,31,31");
			nvram_set("1:ofdmlrbw202gpo", "0");
			nvram_set("1:pa2ga0", "0xfe72,0x14c0,0xfac7");
			nvram_set("1:pa2ga1", "0xfe80,0x1472,0xfabc");
			nvram_set("1:pa2ga2", "0xfe82,0x14bf,0xfad9");
			nvram_set("1:pa5ga0", "0xff31,0x1a56,0xfcc7,0xff35,0x1a8f,0xfcc1,0xff35,0x18d4,0xfcf4,0xff2d,0x18d5,0xfce8");
			nvram_set("1:pa5ga1", "0xff30,0x190f,0xfce6,0xff38,0x1abc,0xfcc0,0xff0f,0x1762,0xfcef,0xff18,0x1648,0xfd23");
			nvram_set("1:pa5ga2", "0xff32,0x18f6,0xfce8,0xff36,0x195d,0xfcdf,0xff28,0x16ae,0xfd1e,0xff28,0x166c,0xfd2b");
			nvram_set("1:papdcap2g", "0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pcieingress_war", "15");
			nvram_set("1:pdgain2g", "4");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0xfecc");
			nvram_set("1:phycal_tempdelta", "255");
			nvram_set("1:rawtempsense", "0x1ff");
//			nvram_set("1:regrev", "0");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgainerr2ga0", "63");
			nvram_set("1:rxgainerr2ga1", "31");
			nvram_set("1:rxgainerr2ga2", "31");
			nvram_set("1:rxgainerr5ga0", "63,63,63,63");
			nvram_set("1:rxgainerr5ga1", "31,31,31,31");
			nvram_set("1:rxgainerr5ga2", "31,31,31,31");
			nvram_set("1:rxgains2gelnagaina0", "0");
			nvram_set("1:rxgains2gelnagaina1", "0");
			nvram_set("1:rxgains2gelnagaina2", "0");
			nvram_set("1:rxgains2gtrisoa0", "0");
			nvram_set("1:rxgains2gtrisoa1", "0");
			nvram_set("1:rxgains2gtrisoa2", "0");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar2g", "18");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:subvid", "0x14e4");
			nvram_set("1:tempcorrx", "0x3f");
			nvram_set("1:tempoffset", "255");
			nvram_set("1:temps_hysteresis", "15");
			nvram_set("1:temps_period", "15");
			nvram_set("1:tempsense_option", "0x3");
			nvram_set("1:tempsense_slope", "0xff");
			nvram_set("1:tempthresh", "255");
			nvram_set("1:tssiposslope2g", "1");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi2g", "0");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14e4");
			nvram_set("1:xtalfreq", "40000");
		}
		break;

	case MODEL_R1D:
		mfr = "Xiaomi";
		name = "MiWiFi";
		features = SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
		nvram_set("usb_usb3", "-1"); /* R1D doesn't have USB 3.0 */
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("boot_wait", "on");	/* failsafe for CFE flash */
			nvram_set("wait_time", "10");	/* failsafe for CFE flash */
			nvram_set("uart_en", "1");	/* failsafe for CFE flash */
			nvram_set("router_name", "X-R1D");
			nvram_set("lan_hostname", "MiWiFi");
			nvram_set("wan_hostname", "MiWiFi");

			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("pci/2/1/macaddr", s);		/* fix WL mac for 2,4G */
			nvram_set("wl1_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("pci/1/1/macaddr", s);		/* fix WL mac for 5G */
			nvram_set("wl0_hwaddr", s);

			/* 5G settings */
			nvram_set("wl0_channel", "36");
			nvram_set("wl0_bw", "3");
			nvram_set("wl0_bw_cap", "7");
			nvram_set("wl0_chanspec", "36/80");
			nvram_set("wl0_nctrlsb", "lower");
			nvram_set("wl0_nband", "1");
			nvram_set("wl0_nbw", "80");
			nvram_set("wl0_nbw_cap", "3");

			/* 2G settings */
			nvram_set("wl1_channel", "6");
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","6l");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("wl1_nband", "2");
			nvram_set("wl1_nbw", "40");
			nvram_set("wl1_nbw_cap", "1");

			/* wifi country settings */
			nvram_set("pci/1/1/regrev", "12");
			nvram_set("pci/2/1/regrev", "12");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");

			nvram_set("wl0_ssid", "FreshTomato50");
			nvram_set("wl1_ssid", "FreshTomato24");

			/* misc wifi settings */
			nvram_set("wl1_vreqd", "0");	/* do not enable vhtmode and vht_features for 2G NON-AC PHY */

#ifdef TCONFIG_BCMBSD
			/* band steering settings correction, because 5 GHz module is the first one */
			nvram_set("wl1_bsd_steering_policy", "0 5 3 -52 0 110 0x22");
			nvram_set("wl0_bsd_steering_policy", "80 5 3 -82 0 0 0x20");
			nvram_set("wl1_bsd_sta_select_policy", "10 -52 0 110 0 1 1 0 0 0 0x122");
			nvram_set("wl0_bsd_sta_select_policy", "10 -82 0 0 0 1 1 0 0 0 0x20");
			nvram_set("wl1_bsd_if_select_policy", "eth1");
			nvram_set("wl0_bsd_if_select_policy", "eth2");
			nvram_set("wl1_bsd_if_qualify_policy", "0 0x0");
			nvram_set("wl0_bsd_if_qualify_policy", "60 0x0");
#endif /* TCONFIG_BCMBSD */

			/* usb settings */
			nvram_set("usb_ohci", "1");     /* USB 1.1 */
			nvram_set("usb_usb3", "-1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* 2.4 GHz module defaults */
			nvram_set("pci/2/1/aa2g", "3");
			nvram_set("pci/2/1/ag0", "2");
			nvram_set("pci/2/1/ag1", "2");
			nvram_set("pci/2/1/antswctl2g", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x80001000");
			nvram_set("pci/2/1/boardflags2", "0x00000800");
			nvram_set("pci/2/1/boardrev", "0x1301");
			nvram_set("pci/2/1/bw40po", "0");
			nvram_set("pci/2/1/bwduppo", "0");
			nvram_set("pci/2/1/bxa2g", "0");
			nvram_set("pci/2/1/cck2gpo", "0x8880");
			nvram_set("pci/2/1/cddpo", "0");
			nvram_set("pci/2/1/devid", "0x43A9");
			nvram_set("pci/2/1/elna2g", "2");
			nvram_set("pci/2/1/extpagain2g", "3");
			nvram_set("pci/2/1/freqoffset_corr", "0");
			nvram_set("pci/2/1/hw_iqcal_en", "0");
			nvram_set("pci/2/1/iqcal_swp_dis", "0");
			nvram_set("pci/2/1/itt2ga1", "32");
			nvram_set("pci/2/1/ledbh0", "255");
			nvram_set("pci/2/1/ledbh1", "255");
			nvram_set("pci/2/1/ledbh2", "255");
			nvram_set("pci/2/1/ledbh3", "131");
			nvram_set("pci/2/1/leddc", "65535");
			nvram_set("pci/2/1/maxp2ga0", "0x2072");
			nvram_set("pci/2/1/maxp2ga1", "0x2072");
			nvram_set("pci/2/1/mcs2gpo0", "0x8888");
			nvram_set("pci/2/1/mcs2gpo1", "0x8888");
			nvram_set("pci/2/1/mcs2gpo2", "0x8888");
			nvram_set("pci/2/1/mcs2gpo3", "0xDDB8");
			nvram_set("pci/2/1/mcs2gpo4", "0x8888");
			nvram_set("pci/2/1/mcs2gpo5", "0xA988");
			nvram_set("pci/2/1/mcs2gpo6", "0x8888");
			nvram_set("pci/2/1/mcs2gpo7", "0xDDC8");
			nvram_set("pci/2/1/measpower1", "0");
			nvram_set("pci/2/1/measpower2", "0");
			nvram_set("pci/2/1/measpower", "0");
			nvram_set("pci/2/1/ofdm2gpo", "0xAA888888");
			nvram_set("pci/2/1/opo", "68");
			nvram_set("pci/2/1/pa2gw0a0", "0xFE77");
			nvram_set("pci/2/1/pa2gw0a1", "0xFE76");
			nvram_set("pci/2/1/pa2gw1a0", "0x1C37");
			nvram_set("pci/2/1/pa2gw1a1", "0x1C5C");
			nvram_set("pci/2/1/pa2gw2a0", "0xF95D");
			nvram_set("pci/2/1/pa2gw2a1", "0xF94F");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdetrange2g", "3");
			nvram_set("pci/2/1/phycal_tempdelta", "0");
			nvram_set("pci/2/1/rawtempsense", "0");
			nvram_set("pci/2/1/rssisav2g", "0");
			nvram_set("pci/2/1/rssismc2g", "0");
			nvram_set("pci/2/1/rssismf2g", "0");
			nvram_set("pci/2/1/rxchain", "3");
			nvram_set("pci/2/1/rxpo2g", "0");
			nvram_set("pci/2/1/sromrev", "8");
			nvram_set("pci/2/1/stbcpo", "0");
			nvram_set("pci/2/1/tempcorrx", "0");
			nvram_set("pci/2/1/tempoffset", "0");
			nvram_set("pci/2/1/temps_hysteresis", "0");
			nvram_set("pci/2/1/temps_period", "0");
			nvram_set("pci/2/1/tempsense_option", "0");
			nvram_set("pci/2/1/tempsense_slope", "0");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/triso2g", "4");
			nvram_set("pci/2/1/tssipos2g", "1");
			nvram_set("pci/2/1/txchain", "3");

			/* 5 GHz module defaults */
			nvram_set("pci/1/1/aa5g", "7");
			nvram_set("pci/1/1/aga0", "01");
			nvram_set("pci/1/1/aga1", "01");
			nvram_set("pci/1/1/aga2", "133");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags", "0x30000000");
			nvram_set("pci/1/1/boardflags2", "0x00300002");
			nvram_set("pci/1/1/boardflags3", "0x00000000");
			nvram_set("pci/1/1/boardvendor", "0x14E4");
			nvram_set("pci/1/1/devid", "0x43B3");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/epagain5g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
			nvram_set("pci/1/1/maxp5ga0", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/maxp5ga1", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/maxp5ga2", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/mcsbw1605ghpo", "0");
			nvram_set("pci/1/1/mcsbw1605glpo", "0");
			nvram_set("pci/1/1/mcsbw1605gmpo", "0");
			nvram_set("pci/1/1/mcsbw205ghpo", "0x55540000");
			nvram_set("pci/1/1/mcsbw205glpo", "0x88642222");
			nvram_set("pci/1/1/mcsbw205gmpo", "0x88642222");
			nvram_set("pci/1/1/mcsbw405ghpo", "0x85542000");
			nvram_set("pci/1/1/mcsbw405glpo", "0xA8842222");
			nvram_set("pci/1/1/mcsbw405gmpo", "0xA8842222");
			nvram_set("pci/1/1/mcsbw805ghpo", "0x85542000");
			nvram_set("pci/1/1/mcsbw805glpo", "0xAA842222");
			nvram_set("pci/1/1/mcsbw805gmpo", "0xAA842222");
			nvram_set("pci/1/1/mcslr5ghpo", "0");
			nvram_set("pci/1/1/mcslr5glpo", "0");
			nvram_set("pci/1/1/mcslr5gmpo", "0");
			nvram_set("pci/1/1/measpower1", "0x7F");
			nvram_set("pci/1/1/measpower2", "0x7F");
			nvram_set("pci/1/1/measpower", "0x7F");
			nvram_set("pci/1/1/pa5ga0", "0xFF90,0x1E37,0xFCB8,0xFF38,0x189B,0xFD00,0xFF33,0x1A66,0xFCC4,0xFF2F,0x1748,0xFD21");
			nvram_set("pci/1/1/pa5ga1", "0xFF1B,0x18A2,0xFCB6,0xFF34,0x183F,0xFD12,0xFF37,0x1AA1,0xFCC0,0xFF2F,0x1889,0xFCFB");
			nvram_set("pci/1/1/pa5ga2", "0xFF1D,0x1653,0xFD33,0xFF38,0x1A2A,0xFCCE,0xFF35,0x1A93,0xFCC1,0xFF3A,0x1ABD,0xFCC0");
			nvram_set("pci/1/1/papdcap5g", "0");
			nvram_set("pci/1/1/pcieingress_war", "15");
			nvram_set("pci/1/1/pdgain5g", "4");
			nvram_set("pci/1/1/pdoffset40ma0", "0x1111");
			nvram_set("pci/1/1/pdoffset40ma1", "0x1111");
			nvram_set("pci/1/1/pdoffset40ma2", "0x1111");
			nvram_set("pci/1/1/pdoffset80ma0", "0");
			nvram_set("pci/1/1/pdoffset80ma1", "0");
			nvram_set("pci/1/1/pdoffset80ma2", "0");
			nvram_set("pci/1/1/phycal_tempdelta", "255");
			nvram_set("pci/1/1/rawtempsense", "0x1FF");
			nvram_set("pci/1/1/rxchain", "7");
			nvram_set("pci/1/1/rxgains5gelnagaina0", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina1", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina2", "1");
			nvram_set("pci/1/1/rxgains5ghelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gmelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/1/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/1/1/sar5g", "15");
			nvram_set("pci/1/1/sb20in40hrpo", "0");
			nvram_set("pci/1/1/sb20in40lrpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80hr5glpo", "0");
			nvram_set("pci/1/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80lr5glpo", "0");
			nvram_set("pci/1/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "4");
			nvram_set("pci/1/1/tempcorrx", "0x3F");
			nvram_set("pci/1/1/tempoffset", "255");
			nvram_set("pci/1/1/temps_hysteresis", "15");
			nvram_set("pci/1/1/temps_period", "15");
			nvram_set("pci/1/1/tempsense_option", "3");
			nvram_set("pci/1/1/tempsense_slope", "255");
			nvram_set("pci/1/1/tempthresh", "255");
			nvram_set("pci/1/1/tssiposslope5g", "1");
			nvram_set("pci/1/1/tworangetssi5g", "0");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14E4");
			nvram_set("pci/1/1/xtalfreq", "0x40000");
		}
		break;
	case MODEL_EA6350v1:
		mfr = "Linksys";
		name = nvram_match("boardnum", "20140309") ? "EA6350v1" : "EA6200";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for wl0 (0:) 2,4G - eth1 for EA6350v1 and/or wl0 (0:) 5G - eth1 for EA6200 */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for wl1 (1:) 5G - eth2 for EA6350v1 and/or wl1 (1:) 2,4G - eth2 for EA6200 */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
			
			if (nvram_match("boardnum", "20140309")) { /* case EA6350v1 */
				/* 2G settings */
				nvram_set("wl0_bw_cap","3");
				nvram_set("wl0_chanspec","6u");
				nvram_set("wl0_channel","6");
				nvram_set("wl0_nbw","40");
				nvram_set("wl0_nctrlsb", "upper");

				/* misc wifi settings */
				nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

				/* 5G settings */
				nvram_set("wl1_bw_cap", "7");
				nvram_set("wl1_chanspec", "36/80");
				nvram_set("wl1_channel", "36");
				nvram_set("wl1_nbw","80");
				nvram_set("wl1_nbw_cap","3");
				nvram_set("wl1_nctrlsb", "lower");
			}
			else { /* case EA6200 */
				/* 5G settings */
				nvram_set("wl0_bw_cap", "7");
				nvram_set("wl0_chanspec", "36/80");
				nvram_set("wl0_channel", "36");
				nvram_set("wl0_nband", "1");
				nvram_set("wl0_nbw","80");
				nvram_set("wl0_nbw_cap","3");
				nvram_set("wl0_nctrlsb", "lower");

				/* 2G settings */
				nvram_set("wl1_bw_cap","3");
				nvram_set("wl1_chanspec","6u");
				nvram_set("wl1_channel","6");
				nvram_set("wl1_nband", "2");
				nvram_set("wl1_nbw","40");
				nvram_set("wl1_nctrlsb", "upper");

				/* misc wifi settings */
				nvram_set("wl1_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

				/* set ssid correct */
				nvram_set("wl0_ssid", "FreshTomato50");
				nvram_set("wl1_ssid", "FreshTomato24");

#ifdef TCONFIG_BCMBSD
				/* band steering settings correction, because 5 GHz module is the first one */
				nvram_set("wl1_bsd_steering_policy", "0 5 3 -52 0 110 0x22");
				nvram_set("wl0_bsd_steering_policy", "80 5 3 -82 0 0 0x20");
				nvram_set("wl1_bsd_sta_select_policy", "10 -52 0 110 0 1 1 0 0 0 0x122");
				nvram_set("wl0_bsd_sta_select_policy", "10 -82 0 0 0 1 1 0 0 0 0x20");
				nvram_set("wl1_bsd_if_select_policy", "eth1");
				nvram_set("wl0_bsd_if_select_policy", "eth2");
				nvram_set("wl1_bsd_if_qualify_policy", "0 0x0");
				nvram_set("wl0_bsd_if_qualify_policy", "60 0x0");
#endif /* TCONFIG_BCMBSD */
			}

			/* 2.4 GHz and 5 GHz defaults */
			/* let the cfe set the init parameter for wifi modules - nothing to modify/adjust right now */
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");

		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;
	case MODEL_EA6400:
		mfr = "Cisco Linksys";
		name = "EA6400";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz module defaults */
			nvram_set("0:aa2g", "3");
			nvram_set("0:ag0", "0");
			nvram_set("0:ag1", "0");
			nvram_set("0:ag2", "0");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags2", "0x00001800");
			nvram_set("0:boardflags", "0x80001200");
			//nvram_set("0:cckbw202gpo", "0x4444");
			//nvram_set("0:cckbw20ul2gpo", "0x4444");
			nvram_set("0:devid", "0x43A9");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:ledbh0", "11");
			nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "11");
			nvram_set("0:ledbh3", "11");
			nvram_set("0:ledbh12", "2");
			nvram_set("0:leddc", "0xFFFF");
			//nvram_set("0:legofdm40duppo", "0x0");
			//nvram_set("0:legofdmbw202gpo", "0x55553300");
			//nvram_set("0:legofdmbw20ul2gpo", "0x55553300");
			nvram_set("0:maxp2ga0", "0x62");
			nvram_set("0:maxp2ga1", "0x62");
			//nvram_set("0:maxp2ga2", "0x62");
			//nvram_set("0:mcs32po", "0x0006");
			//nvram_set("0:mcsbw202gpo", "0xAA997755");
			//nvram_set("0:mcsbw20ul2gpo", "0xAA997755");
			//nvram_set("0:mcsbw402gpo", "0xAA997755");
			nvram_set("0:pa2gw0a0", "0xFE9C");
			nvram_set("0:pa2gw0a1", "0xFEA7");
			//nvram_set("0:pa2gw0a2", "0xFE82");
			nvram_set("0:pa2gw1a0", "0x195C");
			nvram_set("0:pa2gw1a1", "0x1A96");
			//nvram_set("0:pa2gw1a2", "0x1EC5");
			nvram_set("0:pa2gw2a0", "0xFA4B");
			nvram_set("0:pa2gw2a1", "0xFA22");
			//nvram_set("0:pa2gw2a2", "0xF8B8");
			//nvram_set("0:parefldovoltage", "60");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rxchain", "3");
			nvram_set("0:sromrev", "8");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "3");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:xtalfreq", "20000");

			/* 5 GHz module defaults */
			//nvram_set("1:aa2g", "7");
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "0");
			nvram_set("1:aga1", "0");
			nvram_set("1:aga2", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags2", "0x00000002");
			nvram_set("1:boardflags3", "0x0");
			nvram_set("1:boardflags", "0x10000000");
			nvram_set("1:devid", "0x43A2");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "6");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:ledbh10", "2");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:maxp5ga0", "0x4A,0x4A,0x4E,0x4E");
			nvram_set("1:maxp5ga1", "0x4A,0x4A,0x4E,0x4E");
			nvram_set("1:maxp5ga2", "0x4A,0x4A,0x4E,0x4E");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605glpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw205ghpo", "0xEE777700");
			nvram_set("1:mcsbw205glpo", "0xCC888800");
			nvram_set("1:mcsbw205gmpo", "0xCC888800");
			nvram_set("1:mcsbw405ghpo", "0xEE777700");
			nvram_set("1:mcsbw405glpo", "0xCC888800"); 
			nvram_set("1:mcsbw405gmpo", "0xCC888800");
			nvram_set("1:mcsbw805ghpo", "0xEE777700");
			nvram_set("1:mcsbw805glpo", "0xCC888800");
			nvram_set("1:mcsbw805gmpo", "0xCC888800");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:pa5ga0", "0xff5e,0x1418,0xfd78,0xff6c,0x14bc,0xfd77,0xff5d,0x1531,0xfd57,0xff60,0x15a2,0xfd45");
			nvram_set("1:pa5ga1", "0xff7e,0x1527,0xfd7d,0xff75,0x1522,0xfd74,0xff56,0x14bd,0xfd5c,0xff52,0x14c2,0xfd5c");
			nvram_set("1:pa5ga2", "0xff64,0x13f4,0xfd7c,0xff5c,0x13db,0xfd76,0xff5a,0x1473,0xfd5e,0xff61,0x14d2,0xfd5b");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pdgain5g", "10");
			nvram_set("1:pdoffset40ma0", "0x3222");
			nvram_set("1:pdoffset40ma1", "0x3222");
			nvram_set("1:pdoffset40ma2", "0x3222");
			nvram_set("1:pdoffset80ma0", "0x0100");
			nvram_set("1:pdoffset80ma1", "0x0100");
			nvram_set("1:pdoffset80ma2", "0x0100");
			nvram_set("1:phycal_tempdelta", "0");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar2g", "18");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:temps_period", "5");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "40000");
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");

		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;
	case MODEL_EA6700:
		mfr = "Cisco Linksys";
		if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL)
			name = "EA6500v2";
		else
			name = "EA6700";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* misc wifi settings */
			nvram_set("wl0_vreqd", "0"); /* do not enable vhtmode and vht_features for 2G NON-AC PHY */

			/* 2.4 GHz module defaults */
			nvram_set("0:aa2g", "7");
			nvram_set("0:ag0", "0");
			nvram_set("0:ag1", "0");
			nvram_set("0:ag2", "0");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags2", "0x00100000");
			nvram_set("0:boardflags", "0x80001200");
			nvram_set("0:cckbw202gpo", "0x4444");
			nvram_set("0:cckbw20ul2gpo", "0x4444");
			nvram_set("0:devid", "0x4332");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:ledbh0", "11");
			nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "11");
			nvram_set("0:ledbh3", "11");
			nvram_set("0:ledbh12", "2");
			nvram_set("0:leddc", "0xFFFF");
			nvram_set("0:legofdm40duppo", "0x0");
			nvram_set("0:legofdmbw202gpo", "0x55553300");
			nvram_set("0:legofdmbw20ul2gpo", "0x55553300");
			nvram_set("0:maxp2ga0", "0x60");
			nvram_set("0:maxp2ga1", "0x60");
			nvram_set("0:maxp2ga2", "0x60");
			nvram_set("0:mcs32po", "0x0006");
			nvram_set("0:mcsbw202gpo", "0xAA997755");
			nvram_set("0:mcsbw20ul2gpo", "0xAA997755");
			nvram_set("0:mcsbw402gpo", "0xAA997755");
			nvram_set("0:pa2gw0a0", "0xFE7C");
			nvram_set("0:pa2gw0a1", "0xFE85");
			nvram_set("0:pa2gw0a2", "0xFE82");
			nvram_set("0:pa2gw1a0", "0x1E9B");
			nvram_set("0:pa2gw1a1", "0x1EA5");
			nvram_set("0:pa2gw1a2", "0x1EC5");
			nvram_set("0:pa2gw2a0", "0xF8B4");
			nvram_set("0:pa2gw2a1", "0xF8C0");
			nvram_set("0:pa2gw2a2", "0xF8B8");
			nvram_set("0:parefldovoltage", "60");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rxchain", "7");
			nvram_set("0:sromrev", "9");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:xtalfreq", "20000");

			/* 5 GHz module defaults */
			nvram_set("1:aa2g", "7");
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "0");
			nvram_set("1:aga1", "0");
			nvram_set("1:aga2", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags2", "0x00200002");
			nvram_set("1:boardflags3", "0x0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:devid", "0x43A2");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:ledbh10", "2");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:maxp5ga0", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga1", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga2", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605glpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw205ghpo", "0xDD553300");
			nvram_set("1:mcsbw205glpo", "0xDD553300");
			nvram_set("1:mcsbw205gmpo", "0xDD553300");
			nvram_set("1:mcsbw405ghpo", "0xEE885544");
			nvram_set("1:mcsbw405glpo", "0xEE885544"); 
			nvram_set("1:mcsbw405gmpo", "0xEE885544");
			nvram_set("1:mcsbw805ghpo", "0xEE885544");
			nvram_set("1:mcsbw805glpo", "0xEE885544");
			nvram_set("1:mcsbw805gmpo", "0xEE885544");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:pa5ga0", "0xff2b,0x1898,0xfcf2,0xff2c,0x1947,0xfcda,0xff33,0x18f9,0xfcec,0xff2d,0x18ef,0xfce4");
			nvram_set("1:pa5ga1", "0xff31,0x1930,0xfce3,0xff30,0x1974,0xfcd9,0xff31,0x18db,0xfcee,0xff37,0x194e,0xfce1");
			nvram_set("1:pa5ga2", "0xff2e,0x193c,0xfcde,0xff2c,0x1831,0xfcf9,0xff30,0x18c6,0xfcef,0xff30,0x1942,0xfce0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0");
			nvram_set("1:phycal_tempdelta", "0");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar2g", "18");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:temps_period", "5");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "40000");
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");

		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;
	case MODEL_EA6900:
		mfr = "Linksys";
		name = "EA6900";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* 2.4 GHz module defaults */
			nvram_set("0:aa2g", "7");
			nvram_set("0:agbg0", "0x47");
			nvram_set("0:agbg1", "0x47");
			nvram_set("0:agbg2", "0x47");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags", "0x00001000");
			nvram_set("0:boardflags2", "0x00100002");
			nvram_set("0:boardflags3", "0x00000003");
			nvram_set("0:cckbw202gpo", "0x0");
			nvram_set("0:cckbw20ul2gpo", "0x0");
			nvram_set("0:devid", "0x43A1");
			nvram_set("0:dot11agduphrpo", "0x0");
			nvram_set("0:dot11agduplrpo", "0x0");
			nvram_set("0:dot11agofdmhrbw202gpo", "0x6666");
			nvram_set("0:epagain2g", "0");
			nvram_set("0:femctrl", "3");
			nvram_set("0:gainctrlsph", "0");
			nvram_set("0:ledbh0", "0xFF");
			nvram_set("0:ledbh1", "0xFF");
			nvram_set("0:ledbh10", "2");
			nvram_set("0:ledbh2", "0xFF");
			nvram_set("0:ledbh3", "0xFF");
			nvram_set("0:leddc", "0xFFFF");
			nvram_set("0:maxp2ga0", "0x62");
			nvram_set("0:maxp2ga1", "0x62");
			nvram_set("0:maxp2ga2", "0x62");
			nvram_set("0:mcsbw202gpo", "0xCC666600");
			nvram_set("0:mcsbw402gpo", "0xCC666600");
			nvram_set("0:ofdmlrbw202gpo", "0x0");
			nvram_set("0:pa2ga0", "0xff22,0x1a4f,0xfcc1");
			nvram_set("0:pa2ga1", "0xff22,0x1a71,0xfcbb");
			nvram_set("0:pa2ga2", "0xff1f,0x1a21,0xfcc2");
			nvram_set("0:papdcap2g", "0");
			nvram_set("0:parefldovoltage", "35");
			nvram_set("0:pdgain2g", "14");
			nvram_set("0:pdoffset2g40ma0", "0x3");
			nvram_set("0:pdoffset2g40ma1", "0x3");
			nvram_set("0:pdoffset2g40ma2", "0x3");
			nvram_set("0:phycal_tempdelta", "0");
			//nvram_set("0:rpcal2g", "47823");
			nvram_set("0:rpcal2g", "53985");
			nvram_set("0:rxchain", "7");
			nvram_set("0:rxgains2gelnagaina0", "4");
			nvram_set("0:rxgains2gelnagaina1", "4");
			nvram_set("0:rxgains2gelnagaina2", "4");
			nvram_set("0:rxgains2gtrelnabypa0", "1");
			nvram_set("0:rxgains2gtrelnabypa1", "1");
			nvram_set("0:rxgains2gtrelnabypa2", "1");
			nvram_set("0:rxgains2gtrisoa0", "7");
			nvram_set("0:rxgains2gtrisoa1", "7");
			nvram_set("0:rxgains2gtrisoa2", "7");
			nvram_set("0:sb20in40hrpo", "0x0");
			nvram_set("0:sb20in40lrpo", "0x0");
			nvram_set("0:sromrev", "11");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:tssiposslope2g", "1");
			nvram_set("0:tworangetssi2g", "0");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:xtalfreq", "40000");

			/* 5 GHz module defaults */
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "0");
			nvram_set("1:aga1", "0");
			nvram_set("1:aga2", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardflags2", "0x00200002");
			nvram_set("1:boardflags3", "0x00000000");
			nvram_set("1:devid", "0x43A2");
			nvram_set("1:dot11agduphrpo", "0x0");
			nvram_set("1:dot11agduplrpo", "0x0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh10", "2");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:maxp5ga0", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga1", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga2", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:mcsbw205ghpo", "0xBB555500");
			nvram_set("1:mcsbw205glpo", "0xBB555500");
			nvram_set("1:mcsbw205gmpo", "0xBB555500");
			nvram_set("1:mcsbw405ghpo", "0xBB777700");
			nvram_set("1:mcsbw405glpo", "0xBB777700");
			nvram_set("1:mcsbw405gmpo", "0xBB777700");
			nvram_set("1:mcsbw805ghpo", "0xBB777700");
			nvram_set("1:mcsbw805glpo", "0xBB777733");
			nvram_set("1:mcsbw805gmpo", "0xBB777700");
			nvram_set("1:mcslr5ghpo", "0x0");
			nvram_set("1:mcslr5glpo", "0x0");
			nvram_set("1:mcslr5gmpo", "0x0");
			nvram_set("1:pa5ga0", "0xff2e,0x185a,0xfcfc,0xff37,0x1903,0xfcf1,0xff4b,0x197f,0xfcff,0xff37,0x180f,0xfd12");
			nvram_set("1:pa5ga1", "0xff33,0x1944,0xfce5,0xff30,0x18c6,0xfcf5,0xff40,0x19c7,0xfce5,0xff38,0x18cc,0xfcf9");
			nvram_set("1:pa5ga2", "0xff34,0x1962,0xfce1,0xff35,0x193b,0xfceb,0xff38,0x1921,0xfcf1,0xff39,0x188f,0xfd00");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:parefldovoltage", "35");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0xEEEE");
			nvram_set("1:pdoffset80ma1", "0xEEEE");
			nvram_set("1:pdoffset80ma2", "0xEEEE");
			nvram_set("1:phycal_tempdelta", "0");
			//nvram_set("1:rpcal5gb0", "32015");
			//nvram_set("1:rpcal5gb3", "35617");
			nvram_set("1:rpcal5gb0", "41773");
			nvram_set("1:rpcal5gb3", "42547");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sb20in40hrpo", "0x0");
			nvram_set("1:sb20in40lrpo", "0x0");
			nvram_set("1:sb20in80and160hr5ghpo", "0x0");
			nvram_set("1:sb20in80and160hr5glpo", "0x0");
			nvram_set("1:sb20in80and160hr5gmpo", "0x0");
			nvram_set("1:sb20in80and160lr5ghpo", "0x0");
			nvram_set("1:sb20in80and160lr5glpo", "0x0");
			nvram_set("1:sb20in80and160lr5gmpo", "0x0");
			nvram_set("1:sb40and80hr5ghpo", "0x0");
			nvram_set("1:sb40and80hr5glpo", "0x0");
			nvram_set("1:sb40and80hr5gmpo", "0x0");
			nvram_set("1:sb40and80lr5ghpo", "0x0");
			nvram_set("1:sb40and80lr5glpo", "0x0");
			nvram_set("1:sb40and80lr5gmpo", "0x0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:temps_period", "5");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "40000");
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");

		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;
	case MODEL_WZR1750:
		mfr = "Buffalo";
		name = "WZR-1750DHP";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth2 eth1");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");

			/* fix MAC addresses */
			strcpy(s, nvram_safe_get("et0macaddr"));	/* get et0 MAC address for LAN */
			inc_mac(s, +2);					/* MAC + 1 will be for WAN */
			nvram_set("0:macaddr", s);			/* fix WL mac for 2,4G */
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +4);					/* do not overlap with VIFs */
			nvram_set("1:macaddr", s);			/* fix WL mac for 5G */
			nvram_set("wl1_hwaddr", s);

			/* usb3.0 settings */
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			/* misc settings */
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "3");

			/* wifi settings/channels */
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "36/80");
			nvram_set("wl1_channel", "36");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nbw_cap","3");
			nvram_set("wl1_nctrlsb", "lower");

			/* wifi country settings */
			nvram_set("0:regrev", "12");
			nvram_set("1:regrev", "12");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");

			/* 2.4 GHz module defaults */
			nvram_set("devpath0", "pci/2/1");
			nvram_set("0:aa2g", "3");
			nvram_set("0:ag0", "2");
			nvram_set("0:ag1", "2");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags", "0x80001000");
			nvram_set("0:boardflags2", "0x00000800");
			nvram_set("0:boardrev", "0x1301");
			nvram_set("0:bw40po", "0");
			nvram_set("0:bwduppo", "0");
			nvram_set("0:bxa2g", "0");
			nvram_set("0:cck2gpo", "0x8880");
			nvram_set("0:cddpo", "0");
			nvram_set("0:devid", "0x43A9");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:freqoffset_corr", "0");
			nvram_set("0:hw_iqcal_en", "0");
			nvram_set("0:iqcal_swp_dis", "0");
			nvram_set("0:itt2ga1", "32");
			nvram_set("0:ledbh0", "255");
			nvram_set("0:ledbh1", "255");
			nvram_set("0:ledbh2", "255");
			nvram_set("0:ledbh3", "131");
			nvram_set("0:ledbh12", "7");
			nvram_set("0:leddc", "65535");
			nvram_set("0:maxp2ga0", "0x2072");
			nvram_set("0:maxp2ga1", "0x2072");
			nvram_set("0:mcs2gpo0", "0x8888");
			nvram_set("0:mcs2gpo1", "0x8888");
			nvram_set("0:mcs2gpo2", "0x8888");
			nvram_set("0:mcs2gpo3", "0xDDB8");
			nvram_set("0:mcs2gpo4", "0x8888");
			nvram_set("0:mcs2gpo5", "0xA988");
			nvram_set("0:mcs2gpo6", "0x8888");
			nvram_set("0:mcs2gpo7", "0xDDC8");
			nvram_set("0:measpower1", "0");
			nvram_set("0:measpower2", "0");
			nvram_set("0:measpower", "0");
			nvram_set("0:ofdm2gpo", "0xAA888888");
			nvram_set("0:opo", "68");
			nvram_set("0:pa2gw0a0", "0xFE77");
			nvram_set("0:pa2gw0a1", "0xFE76");
			nvram_set("0:pa2gw1a0", "0x1C37");
			nvram_set("0:pa2gw1a1", "0x1C5C");
			nvram_set("0:pa2gw2a0", "0xF95D");
			nvram_set("0:pa2gw2a1", "0xF94F");
			nvram_set("0:pcieingress_war", "15");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rawtempsense", "0");
			nvram_set("0:rssisav2g", "0");
			nvram_set("0:rssismc2g", "0");
			nvram_set("0:rssismf2g", "0");
			nvram_set("0:rxchain", "3");
			nvram_set("0:rxpo2g", "0");
			nvram_set("0:sromrev", "8");
			nvram_set("0:stbcpo", "0");
			nvram_set("0:tempcorrx", "0");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "0");
			nvram_set("0:temps_period", "0");
			nvram_set("0:tempsense_option", "0");
			nvram_set("0:tempsense_slope", "0");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:triso2g", "4");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "3");

			/* 5 GHz module defaults */
			nvram_set("devpath1", "pci/1/1");
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "01");
			nvram_set("1:aga1", "01");
			nvram_set("1:aga2", "133");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardflags2", "0x00300002");
			nvram_set("1:boardflags3", "0x00000000");
			nvram_set("1:boardvendor", "0x14E4");
			nvram_set("1:devid", "0x43B3");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh10", "7");
			nvram_set("1:maxp5ga0", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:maxp5ga1", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:maxp5ga2", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605glpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw205ghpo", "0x55540000");
			nvram_set("1:mcsbw205glpo", "0x88642222");
			nvram_set("1:mcsbw205gmpo", "0x88642222");
			nvram_set("1:mcsbw405ghpo", "0x85542000");
			nvram_set("1:mcsbw405glpo", "0xA8842222");
			nvram_set("1:mcsbw405gmpo", "0xA8842222");
			nvram_set("1:mcsbw805ghpo", "0x85542000");
			nvram_set("1:mcsbw805glpo", "0xAA842222");
			nvram_set("1:mcsbw805gmpo", "0xAA842222");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:measpower1", "0x7F");
			nvram_set("1:measpower2", "0x7F");
			nvram_set("1:measpower", "0x7F");
			nvram_set("1:pa5ga0", "0xFF90,0x1E37,0xFCB8,0xFF38,0x189B,0xFD00,0xFF33,0x1A66,0xFCC4,0xFF2F,0x1748,0xFD21");
			nvram_set("1:pa5ga1", "0xFF1B,0x18A2,0xFCB6,0xFF34,0x183F,0xFD12,0xFF37,0x1AA1,0xFCC0,0xFF2F,0x1889,0xFCFB");
			nvram_set("1:pa5ga2", "0xFF1D,0x1653,0xFD33,0xFF38,0x1A2A,0xFCCE,0xFF35,0x1A93,0xFCC1,0xFF3A,0x1ABD,0xFCC0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pcieingress_war", "15");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0");
			nvram_set("1:phycal_tempdelta", "255");
			nvram_set("1:rawtempsense", "0x1FF");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempcorrx", "0x3F");
			nvram_set("1:tempoffset", "255");
			nvram_set("1:temps_hysteresis", "15");
			nvram_set("1:temps_period", "15");
			nvram_set("1:tempsense_option", "3");
			nvram_set("1:tempsense_slope", "255");
			nvram_set("1:tempthresh", "255");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "0x40000");
		}
		break;
#endif /* CONFIG_BCMWL6A */
#endif /* WL_BSS_INFO_VERSION >= 108 */
	} /* switch (model) */

	wl_defaults(); /* check and align wifi values */
	
	if (name) {
		nvram_set("t_fix1", name);
		if (ver && strcmp(ver, "")) {
			sprintf(s, "%s %s v%s", mfr, name, ver);
		} else {
			sprintf(s, "%s %s", mfr, name);
		}
	}
	else {
		snprintf(s, sizeof(s), "%s %d/%s/%s/%s/%s", mfr, check_hw_type(),
			nvram_safe_get("boardtype"), nvram_safe_get("boardnum"), nvram_safe_get("boardrev"), nvram_safe_get("boardflags"));
		s[64] = 0;
	}
	nvram_set("t_model_name", s);

	sprintf(s, "0x%lX", features);
	nvram_set("t_features", s);

	/*
	 * note: set wan_ifnameX if wan_ifname needs to be overriden
	 */

	if (nvram_is_empty("wan_ifnameX")) {
		nvram_set("wan_ifnameX", (strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ? "vlan1" : "eth1");
	}

	nvram_set("wan_get_dns", "");
	nvram_set("wan_get_domain", "");
	nvram_set("wan_ppp_get_ip", "0.0.0.0");
	nvram_set("action_service", "");
	nvram_set("jffs2_format", "0");
	nvram_set("rrules_radio", "-1");
	nvram_unset("https_crt_gen");
	nvram_unset("log_wmclear");
#ifdef TCONFIG_IPV6
	nvram_set("ipv6_get_dns", "");
#endif
#ifdef TCONFIG_MEDIA_SERVER
	nvram_unset("ms_rescan");
#endif
	if (nvram_get_int("http_id_gen") == 1) nvram_unset("http_id");

	nvram_unset("sch_rboot_last");
	nvram_unset("sch_rcon_last");
	nvram_unset("sch_c1_last");
	nvram_unset("sch_c2_last");
	nvram_unset("sch_c3_last");

	nvram_set("brau_state", "");
	if ((features & SUP_BRAU) == 0) nvram_set("script_brau", "");
	if ((features & SUP_SES) == 0) nvram_set("sesx_script", "");

	if ((features & SUP_1000ET) == 0) nvram_set("jumbo_frame_enable", "0");

	/* compatibility with old versions */
	if (nvram_match("wl_net_mode", "disabled")) {
		nvram_set("wl_radio", "0");
		nvram_set("wl_net_mode", "mixed");
	}

	return 0;
}

#ifndef TCONFIG_BCMARM
/* Get the special files from nvram and copy them to disc.
 * These were files saved with "nvram setfile2nvram <filename>".
 * Better hope that they were saved with full pathname.
 */
static void load_files_from_nvram(void)
{
	char *name, *cp;
	int ar_loaded = 0;
	char buf[NVRAM_SPACE];

	if (nvram_getall(buf, sizeof(buf)) != 0)
		return;

	for (name = buf; *name; name += strlen(name) + 1) {
		if (strncmp(name, "FILE:", 5) == 0) { /* This special name marks a file to get. */
			if ((cp = strchr(name, '=')) == NULL)
				continue;
			*cp = 0;
			logmsg(LOG_INFO, "loading file '%s' from nvram", name + 5);
			nvram_nvram2file(name, name + 5);
			if (memcmp(".autorun", cp - 8, 9) == 0) 
				++ar_loaded;
		}
	}
	/* Start any autorun files that may have been loaded into one of the standard places. */
	if (ar_loaded != 0)
		run_nvscript(".autorun", NULL, 3);
}
#endif

static inline void set_jumbo_frame(void)
{
	int enable = nvram_get_int("jumbo_frame_enable");

	/*
	 * 0x40 JUMBO frame page
	 * JUMBO Control Register
	 * 0x01 REG_JUMBO_CTRL (Port Mask (bit i == port i enabled), bit 24 == GigE always enabled)
	 * 0x05 REG_JUMBO_SIZE
	 */
#ifdef TCONFIG_BCMARM
	eval("et", "robowr", "0x40", "0x01", enable ? "0x010001ff" : "0x00", "4"); /* set enable flag for arm (32 bit) */
#else
	/* at mips branch we set the enable flag arleady at bcmrobo.c --> so nothing to do here right now */
	//eval("et", "robowr", "0x40", "0x01", enable ? "0x1f" : "0x00"); /* set enable flag for mips */
#endif
	if (enable) {
		eval("et", "robowr", "0x40", "0x05", nvram_safe_get("jumbo_frame_size")); /* set the packet size */
	}
}

static inline void set_kernel_panic(void)
{
	/* automatically reboot after a kernel panic */
	f_write_string("/proc/sys/kernel/panic", "3", 0, 0);
	f_write_string("/proc/sys/kernel/panic_on_oops", "3", 0, 0);
}

static inline void set_kernel_memory(void)
{
	f_write_string("/proc/sys/vm/overcommit_memory", "2", 0, 0); /* Linux kernel will not overcommit memory */
	f_write_string("/proc/sys/vm/overcommit_ratio", "75", 0, 0); /* allow userspace to commit up to 75% of total memory */
}

#ifdef TCONFIG_USB
static inline void tune_min_free_kbytes(void)
{
	struct sysinfo info;

	memset(&info, 0, sizeof(struct sysinfo));
	sysinfo(&info);
	if (info.totalram >= (TOMATO_RAM_HIGH_END * 1024)) { /* Router with 256 MB RAM and more */
		f_write_string("/proc/sys/vm/min_free_kbytes", "20480", 0, 0);  /* 20 MByte */
	}
	else if (info.totalram >= (TOMATO_RAM_MID_END * 1024)) { /* Router with 128 MB RAM */
		f_write_string("/proc/sys/vm/min_free_kbytes", "14336", 0, 0); /* 14 MByte */
	}
	else if (info.totalram >= (TOMATO_RAM_LOW_END * 1024)) {
		/* If we have 64MB+ RAM, tune min_free_kbytes
		 * to reduce page allocation failure errors.
		 */
		f_write_string("/proc/sys/vm/min_free_kbytes", "8192", 0, 0); /* 8 MByte */
	}
}
#endif

/*
Example for RT-AC56U (default, without tune smp_affinity)
# cat /proc/interrupts
           CPU0       CPU1
 27:         33          0         GIC  mpcore_gtimer
 32:          0          0         GIC  L2C
111:          0          0         GIC  ehci_hcd:usb2
112:          1          0         GIC  xhci_hcd:usb1
117:        240          0         GIC  serial
163:      20012          0         GIC  eth1
169:       3337          0         GIC  eth2
179:     171514          0         GIC  eth0
IPI:       3266       3852
LOC:      60178      62030
Err:          0

Example for AC3200 (default, without tune smp_affinity)
# cat /proc/interrupts
           CPU0       CPU1
 27:         36          0         GIC  mpcore_gtimer
 32:          0          0         GIC  L2C
111:    2824873          0         GIC  ehci_hcd:usb1
117:     124872          0         GIC  serial
163:        552    3558156         GIC  dhdpcie:0001:03:00.0, dhdpcie:0001:04:00.0
169:          0     138911         GIC  dhdpcie:0002:01:00.0
179:     390168          0         GIC  eth0
IPI:     207105     908084
LOC:    4037843    3667045
Err:          0
 */

#if defined(TCONFIG_BCMSMP) && defined(TCONFIG_USB)
static inline void tune_smp_affinity(void)
{
	int fd;
	int mode;

	mode = nvram_get_int("smbd_enable");

	if ((fd = open("/proc/irq/163/smp_affinity", O_RDWR)) >= 0) {
		close(fd);

		if (mode) {	/* samba is enabled */
			f_write_string("/proc/irq/179/smp_affinity", TOMATO_CPUX, 0, 0);	/* eth0 --> CPU 0 and 1 (no change, default) */
			f_write_string("/proc/irq/163/smp_affinity", TOMATO_CPU1, 0, 0);	/* eth1 --> CPU 1 (assign at least 163 to CPU1) */
			f_write_string("/proc/irq/169/smp_affinity", TOMATO_CPUX, 0, 0);	/* eth2 --> CPU 0 and 1 (no change, default) */
		}
		else {
			f_write_string("/proc/irq/179/smp_affinity", TOMATO_CPU0, 0, 0);	/* eth0 --> CPU 0 */
			f_write_string("/proc/irq/163/smp_affinity", TOMATO_CPU1, 0, 0);	/* eth1 --> CPU 1 */
			f_write_string("/proc/irq/169/smp_affinity", TOMATO_CPU1, 0, 0);	/* eth2 --> CPU 1 */
		}
	}

}
#endif

static void sysinit(void)
{
	static int noconsole = 0;
	static const time_t tm = 0;
	unsigned int i;
	DIR *d;
	struct dirent *de;
	char s[256];
	char t[256];
#if defined(TCONFIG_USB)
	int mode;
#endif

	mount("proc", "/proc", "proc", 0, NULL);
	mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
	mount("devfs", "/dev", "tmpfs", MS_MGC_VAL | MS_NOATIME, NULL);
	mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
	mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1));
	mount("sysfs", "/sys", "sysfs", MS_MGC_VAL, NULL);
	mkdir("/dev/shm", 0777);
	mkdir("/dev/pts", 0777);
	mknod("/dev/pts/ptmx", S_IRWXU|S_IFCHR, makedev(5, 2));
	mknod("/dev/pts/0", S_IRWXU|S_IFCHR, makedev(136, 0));
	mknod("/dev/pts/1", S_IRWXU|S_IFCHR, makedev(136, 1));
	mount("devpts", "/dev/pts", "devpts", MS_MGC_VAL, NULL);

	if (console_init()) noconsole = 1;

	stime(&tm);

	static const char *mkd[] = {
		"/tmp/etc", "/tmp/var", "/tmp/home", "/tmp/mnt",
		"/tmp/splashd", /* !!Victek */
		"/tmp/share", "/var/webmon", /* !!TB */
		"/var/log", "/var/run", "/var/tmp", "/var/lib", "/var/lib/misc",
		"/var/spool", "/var/spool/cron", "/var/spool/cron/crontabs",
		"/tmp/var/wwwext", "/tmp/var/wwwext/cgi-bin",	/* !!TB - CGI support */
		NULL
	};
	umask(0);
	for (i = 0; mkd[i]; ++i) {
		mkdir(mkd[i], 0755);
	}
	mkdir("/var/lock", 0777);
	mkdir("/var/tmp/dhcp", 0777);
	mkdir("/home/root", 0700);
	chmod("/tmp", 0777);
	f_write("/etc/hosts", NULL, 0, 0, 0644);			/* blank */
	f_write("/etc/fstab", NULL, 0, 0, 0644);			/* !!TB - blank */
	simple_unlock("cron");
	simple_unlock("firewall");
	simple_unlock("restrictions");
	umask(022);

	if ((d = opendir("/rom/etc")) != NULL) {
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') continue;
			snprintf(s, sizeof(s), "%s/%s", "/rom/etc", de->d_name);
			snprintf(t, sizeof(t), "%s/%s", "/etc", de->d_name);
			symlink(s, t);
		}
		closedir(d);
	}
	symlink("/proc/mounts", "/etc/mtab");

#ifdef TCONFIG_SAMBASRV
	if ((d = opendir("/usr/codepages")) != NULL) {
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') continue;
			snprintf(s, sizeof(s), "/usr/codepages/%s", de->d_name);
			snprintf(t, sizeof(t), "/usr/share/%s", de->d_name);
			symlink(s, t);
		}
		closedir(d);
	}
#endif

	static const char *dn[] = {
		"null", "zero", "random", "urandom", "full", "ptmx", "nvram",
		NULL
	};
	for (i = 0; dn[i]; ++i) {
		snprintf(s, sizeof(s), "/dev/%s", dn[i]);
		chmod(s, 0666);
	}
	chmod("/dev/gpio", 0660);

	set_action(ACT_IDLE);

	for (i = 0; defenv[i]; ++i) {
		putenv(defenv[i]);
	}

	eval("hotplug2", "--coldplug");
	start_hotplug2();

	if (!noconsole) {
		printf("\n\nHit ENTER for console...\n\n");
		run_shell(1, 0);
	}

	check_bootnv();

#ifdef TCONFIG_IPV6
	/* disable IPv6 by default on all interfaces */
	f_write_procsysnet("ipv6/conf/default/disable_ipv6", "1");
#endif

	for (i = 0; i < sizeof(fatalsigs) / sizeof(fatalsigs[0]); i++) {
		signal(fatalsigs[i], handle_fatalsigs);
	}
	signal(SIGCHLD, handle_reap);

#ifdef CONFIG_BCMWL5
	/* ctf must be loaded prior to any other modules */
	if (nvram_invmatch("ctf_disable", "1"))
		modprobe("ctf");
#endif

#if defined(TCONFIG_USB)
	mode = nvram_get_int("smbd_enable");

	/* check samba enabled ? */
	if (mode) {
	  nvram_set("txworkq", "1");	/* set txworkq to 1, see et/sys/et_linux.c */
	}
	else {
	  nvram_unset("txworkq");
	}
#endif

#ifdef TCONFIG_EMF
	modprobe("emf");
	modprobe("igs");
#endif

	modprobe("et");

	restore_defaults(); /* restore default if necessary */
	init_nvram();

	set_jumbo_frame(); /* enable or disable jumbo_frame and set jumbo frame size */

	/* load after init_nvram */
#ifdef TCONFIG_BCM7
#ifdef TCONFIG_DHDAP
	load_wl(); /* for sdk6 see function start_lan() */
#endif
#endif

	//config_loopback(); /* see function start_lan() */

	klogctl(8, NULL, nvram_get_int("console_loglevel"));

#ifdef TCONFIG_USB
	tune_min_free_kbytes();
#endif

#if defined(TCONFIG_BCMSMP) && defined(TCONFIG_USB)
	tune_smp_affinity();
#endif

	set_kernel_panic(); /* Reboot automatically when the kernel panics and set waiting time */
	set_kernel_memory(); /* set overcommit_memory and overcommit_ratio */

	setup_conntrack();
	set_host_domain_name();

	set_tz();

	eval("buttons");

	/* stealth mode */
	if (nvram_match("stealth_mode", "0")) { /* start blink_br only if stealth mode is off */
	  /* enable LED for LAN / Bridge */
	  eval("blink_br");
	}

	if (!noconsole) xstart("console");

	/* Startup LED setup, see GUI (admin-buttons.asp) */
	i = nvram_get_int("sesx_led");
	led(LED_AMBER, (i & 1) != 0);
	led(LED_WHITE, (i & 2) != 0);
	led(LED_AOSS, (i & 4) != 0);
	led(LED_BRIDGE, (i & 8) != 0);
	led(LED_DIAG, LED_ON);
}

int init_main(int argc, char *argv[])
{
	unsigned int i;
	int state;
	sigset_t sigset;

	/* AB - failsafe? */
	nvram_unset("debug_rc_svc");

	sysinit();

	sigemptyset(&sigset);
	for (i = 0; i < sizeof(initsigs) / sizeof(initsigs[0]); i++) {
		sigaddset(&sigset, initsigs[i]);
	}
	sigprocmask(SIG_BLOCK, &sigset, NULL);

#if defined(DEBUG_NOISY)
	nvram_set("debug_logeval", "1");
	nvram_set("debug_cprintf", "1");
	nvram_set("debug_cprintf_file", "1");
	nvram_set("debug_ddns", "1");
#endif

	/* reset ntp status */
	nvram_set("ntp_ready", "0");

	start_jffs2();

	/* set unique system id */
	if (!f_exists("/etc/machine-id"))
		system("echo $(nvram get lan_hwaddr) | md5sum | cut -b -32 > /etc/machine-id");

	state = SIGUSR2;	/* START */

	for (;;) {
		switch (state) {
		case SIGUSR1:		/* USER1: service handler */
			exec_service();
			break;

		case SIGHUP:		/* RESTART */
		case SIGINT:		/* STOP */
		case SIGQUIT:		/* HALT */
		case SIGTERM:		/* REBOOT */
			led(LED_DIAG, LED_ON);
			unlink("/var/notice/sysup");

			if (nvram_match( "webmon_bkp", "1" )) {
				xstart( "/usr/sbin/webmon_bkp", "hourly" ); /* make a copy before halt/reboot router */
			}

			run_nvscript("script_shut", NULL, 10);

			stop_services();
			stop_wan();
			stop_arpbind();
			stop_lan();
			stop_vlan();

			if ((state == SIGTERM /* REBOOT */) ||
			    (state == SIGQUIT /* HALT */)) {
				remove_storage_main(1);
				stop_usb();
				stop_syslog();

				shutdn(state == SIGTERM /* REBOOT */);
				sync(); sync(); sync();
				exit(0);
			}
			if (state == SIGINT /* STOP */) {
				break;
			}

			/* SIGHUP (RESTART) falls through */

			//nvram_set("wireless_restart_req", "1"); /* restart wifi twice to make sure all is working ok! not needed right now M_ars */
			logmsg(LOG_INFO, "FreshTomato RESTART ...");

		case SIGUSR2:		/* START */
			stop_syslog();
			start_syslog();

#ifndef TCONFIG_BCMARM
			load_files_from_nvram();
#endif

			int fd = -1;
			fd = file_lock("usb"); /* hold off automount processing */
			start_usb();

			xstart("/usr/sbin/mymotd", "init");
			run_nvscript("script_init", NULL, 2);

			file_unlock(fd); /* allow to process usb hotplug events */
#ifdef TCONFIG_USB
			/*
			 * On RESTART some partitions can stay mounted if they are busy at the moment.
			 * In that case USB drivers won't unload, and hotplug won't kick off again to
			 * remount those drives that actually got unmounted. Make sure to remount ALL
			 * partitions here by simulating hotplug event.
			 */
			if (state == SIGHUP) /* RESTART */
				add_remove_usbhost("-1", 1);
#endif

			log_segfault();
			create_passwd();
			start_vlan();
			start_lan();
			start_arpbind();
			mwan_state_files();
			start_services();

			if (restore_defaults_fb /*|| nvram_match("wireless_restart_req", "1")*/) {
				logmsg(LOG_INFO, "%s: FreshTomato WiFi restarting ... (restore defaults)", nvram_safe_get("t_model_name"));
				restore_defaults_fb = 0; /* reset */
				//nvram_set("wireless_restart_req", "0");
				restart_wireless();
			}
			else {
				start_wl();
#ifdef CONFIG_BCMWL5
				/* If a virtual SSID is disabled, it requires two initialisations */
				if (foreach_wif(1, NULL, disabled_wl)) {
					logmsg(LOG_INFO, "%s: FreshTomato WiFi restarting ... (virtual SSID disabled)", nvram_safe_get("t_model_name"));
					restart_wireless();
				}
#endif
			}
			/*
			 * last one as ssh telnet httpd samba etc can fail to load until start_wan_done
			 */
			start_wan();

#ifdef CONFIG_BCMWL5
			if (wds_enable()) {
				/* Restart NAS one more time - for some reason without
				 * this the new driver doesn't always bring WDS up.
				 */
				stop_nas();
				start_nas();
			}
#endif

			logmsg(LOG_INFO, "%s: FreshTomato %s", nvram_safe_get("t_model_name"), tomato_version);

			led(LED_DIAG, LED_OFF);
			notice_set("sysup", "");
			break;
		}

		if (!g_upgrade) {
			chld_reap(0); /* Periodically reap zombies. */
			check_services();
		}

		sigwait(&sigset, &state);
	}

	return 0;
}

int reboothalt_main(int argc, char *argv[])
{
	int reboot = (strstr(argv[0], "reboot") != NULL);
	int def_reset_wait = 30;

	cprintf(reboot ? "Rebooting...\n" : "Shutting down...\n");
	kill(1, reboot ? SIGTERM : SIGQUIT);

	int wait = nvram_get_int("reset_wait") ? : def_reset_wait;
	/* In the case we're hung, we'll get stuck and never actually reboot.
	 * The only way out is to pull power.
	 * So after 'reset_wait' seconds (default: 30), forcibly crash & restart.
	 */
	if (fork() == 0) {
		if ((wait < 10) || (wait > 120))
			wait = 10;

		f_write("/proc/sysrq-trigger", "s", 1, 0 , 0); /* sync disks */
		sleep(wait);
		cprintf("Still running... Doing machine reset.\n");
#ifdef TCONFIG_USB
		remove_usb_module();
#endif
		f_write("/proc/sysrq-trigger", "s", 1, 0 , 0); /* sync disks */
		sleep(1);
		f_write("/proc/sysrq-trigger", "b", 1, 0 , 0); /* machine reset */
	}

	return 0;
}
