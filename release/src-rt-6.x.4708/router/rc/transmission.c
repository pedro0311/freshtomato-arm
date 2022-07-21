/*
 * transmission.c
 *
 * Copyright (C) 2011 shibby
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#include <sys/types.h>
#include <sys/wait.h>

#define tr_dir			"/etc/transmission"
#define tr_settings		tr_dir"/settings.json"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"transmission_debug"

static int rmem_max = 0;
static int wmem_max = 0;
static pid_t pidof_child = 0;


void start_bittorrent(int force)
{
	FILE *fp;
	char *pb, *pc, *pd, *pe, *pf, *ph, *pi, *pj, *pk, *pl, *pm, *pn, *po, *pp, *pr, *pt, *pu;
	char *whitelistEnabled;
	char buf[256], buf2[64];
	int n;

	/* only if enabled or forced */
	if (!nvram_get_int("bt_enable") && force == 0)
		return;

	if (serialize_restart("transmission-da", 1))
		return;

	if (pidof_child > 0) { /* fork is still up */
		logmsg(LOG_WARNING, "*** %s: another process (PID: %d) still up, aborting ...", __FUNCTION__, pidof_child);
		return;
	}

	logmsg(LOG_INFO, "starting transmission-daemon ...");

	/* collecting data */
	if (nvram_get_int("bt_rpc_enable"))              { pb = "true"; } else { pb = "false"; }
	if (nvram_get_int("bt_dl_enable"))               { pc = "true"; } else { pc = "false"; }
	if (nvram_get_int("bt_ul_enable"))               { pd = "true"; } else { pd = "false"; }
	if (nvram_get_int("bt_incomplete"))              { pe = "true"; } else { pe = "false"; }
	if (nvram_get_int("bt_autoadd"))                 { pf = "true"; } else { pf = "false"; }
	if (nvram_get_int("bt_ratio_enable"))            { ph = "true"; } else { ph = "false"; }
	if (nvram_get_int("bt_dht"))                     { pi = "true"; } else { pi = "false"; }
	if (nvram_get_int("bt_pex"))                     { pj = "true"; } else { pj = "false"; }
	if (nvram_get_int("bt_blocklist"))               { pm = "true"; } else { pm = "false"; }
	if (nvram_get_int("bt_lpd"))                     { po = "true"; } else { po = "false"; }
	if (nvram_get_int("bt_utp"))                     { pp = "true"; } else { pp = "false"; }
	if (nvram_get_int("bt_ratio_idle_enable"))       { pr = "true"; } else { pr = "false"; }
	if (nvram_get_int("bt_dl_queue_enable"))         { pt = "true"; } else { pt = "false"; }
	if (nvram_get_int("bt_ul_queue_enable"))         { pu = "true"; } else { pu = "false"; }

	if      (nvram_match("bt_settings", "down_dir")) { pk = nvram_safe_get("bt_dir"); }
	else if (nvram_match("bt_settings", "custom"))   { pk = nvram_safe_get("bt_settings_custom"); }
	else                                             { pk = nvram_safe_get("bt_settings"); }

	if      (nvram_match("bt_binary", "internal"))   { pn = "/usr/bin"; }
	else if (nvram_match("bt_binary", "optware") )   { pn = "/opt/bin"; }
	else                                             { pn = nvram_safe_get( "bt_binary_custom"); }

	if (nvram_get_int("bt_auth")) {
		pl = "true";
		whitelistEnabled = "false";
	}
	else {
		pl = "false";
		whitelistEnabled = "true";
	}

	/* writing data to file */
	mkdir_if_none(tr_dir);
	if (!(fp = fopen(tr_settings, "w"))) {
		perror(tr_settings);
		return;
	}

	fprintf(fp, "{\n"
	            "\"peer-port\": %s,\n"
	            "\"speed-limit-down-enabled\": %s,\n"
	            "\"speed-limit-up-enabled\": %s,\n"
	            "\"speed-limit-down\": %s,\n"
	            "\"speed-limit-up\": %s,\n"
	            "\"rpc-enabled\": %s,\n"
	            "\"rpc-bind-address\": \"0.0.0.0\",\n"
	            "\"rpc-port\": %s,\n"
	            "\"rpc-whitelist\": \"*\",\n"
	            "\"rpc-whitelist-enabled\": %s,\n"
	            "\"rpc-host-whitelist\": \"*\",\n"
	            "\"rpc-host-whitelist-enabled\": %s,\n"
	            "\"rpc-username\": \"%s\",\n"
	            "\"rpc-password\": \"%s\",\n"
	            "\"download-dir\": \"%s\",\n"
	            "\"incomplete-dir-enabled\": \"%s\",\n"
	            "\"incomplete-dir\": \"%s/.incomplete\",\n"
	            "\"watch-dir\": \"%s\",\n"
	            "\"watch-dir-enabled\": %s,\n"
	            "\"peer-limit-global\": %s,\n"
	            "\"peer-limit-per-torrent\": %s,\n"
	            "\"upload-slots-per-torrent\": %s,\n"
	            "\"dht-enabled\": %s,\n"
	            "\"pex-enabled\": %s,\n"
	            "\"lpd-enabled\": %s,\n"
	            "\"utp-enabled\": %s,\n"
	            "\"ratio-limit-enabled\": %s,\n"
	            "\"ratio-limit\": %s,\n"
	            "\"idle-seeding-limit-enabled\": %s,\n"
	            "\"idle-seeding-limit\": %s,\n"
	            "\"blocklist-enabled\": %s,\n"
	            "\"blocklist-url\": \"%s\",\n"
	            "\"download-queue-enabled\": %s,\n"
	            "\"download-queue-size\": %s,\n"
	            "\"seed-queue-enabled\": %s,\n"
	            "\"seed-queue-size\": %s,\n"
	            "\"message-level\": %s,\n"
	            "%s%s"
	            "\"rpc-authentication-required\": %s\n"
	            "}\n",
	            nvram_safe_get( "bt_port"),
	            pc,
	            pd,
	            nvram_safe_get("bt_dl"),
	            nvram_safe_get("bt_ul"),
	            pb,
	            nvram_safe_get("bt_port_gui"),
	            whitelistEnabled,
	            whitelistEnabled,
	            nvram_safe_get("bt_login"),
	            nvram_safe_get("bt_password"),
	            nvram_safe_get("bt_dir"),
	            pe,
	            nvram_safe_get("bt_dir"),
	            nvram_safe_get("bt_dir"),
	            pf,
	            nvram_safe_get("bt_peer_limit_global"),
	            nvram_safe_get("bt_peer_limit_per_torrent"),
	            nvram_safe_get("bt_ul_slot_per_torrent"),
	            pi,
	            pj,
	            po,
	            pp,
	            ph,
	            nvram_safe_get("bt_ratio"),
	            pr,
	            nvram_safe_get("bt_ratio_idle"),
	            pm,
	            nvram_safe_get("bt_blocklist_url"),
	            pt,
	            nvram_safe_get("bt_dl_queue_size"),
	            pu,
	            nvram_safe_get("bt_ul_queue_size"),
	            nvram_safe_get("bt_message"),
	            nvram_safe_get("bt_custom"),
	            (strcmp(nvram_safe_get("bt_custom"), "") ? ",\n" : ""),
	            pl);

	fclose(fp);

	chmod(tr_settings, 0644);

	/* backup original buffers values */
	if (rmem_max == 0) {
		memset(buf, 0, sizeof(buf));
		f_read_string("/proc/sys/net/core/rmem_max", buf, sizeof(buf));
		rmem_max = atoi(buf);
	}
	if (wmem_max == 0) {
		memset(buf, 0, sizeof(buf));
		f_read_string("/proc/sys/net/core/wmem_max", buf, sizeof(buf));
		wmem_max = atoi(buf);
	}

	/* tune buffers */
	f_write_procsysnet("core/rmem_max", "4194304");
	f_write_procsysnet("core/wmem_max", "2080768");

	/* fork new process */
	if (fork() != 0)
		return;

	pidof_child = getpid();

	/* wait a given time for partition to be mounted, etc */
	n = atoi(nvram_safe_get("bt_sleep"));
	if (n > 0)
		sleep(n);

	/* only now prepare subdirs */
	mkdir_if_none(pk);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s/.settings", pk);
	mkdir_if_none(buf);
	if (nvram_get_int("bt_incomplete")) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s/.incomplete", pk);
		mkdir_if_none(buf);
	}

	/* TBD: need better regex, trim triple commas, be safe for passwords etc */
	eval("sed", "-i", "s/,,\\s/, /g", tr_settings);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "cp "tr_settings" %s/.settings", pk);
	system(buf);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s/.settings/blocklists/*", pk);
	remove(buf);

	memset(buf, 0, sizeof(buf));
	if (nvram_get_int("bt_blocklist")) {
#ifdef TCONFIG_STUBBY
		snprintf(buf, sizeof(buf), "wget %s -O %s/.settings/blocklists/level1.gz", nvram_safe_get("bt_blocklist_url"), pk);
#else
		snprintf(buf, sizeof(buf), "wget --no-check-certificate %s -O %s/.settings/blocklists/level1.gz", nvram_safe_get("bt_blocklist_url"), pk);
#endif
		system(buf);

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "gunzip %s/.settings/blocklists/level1.gz", pk);
		system(buf);
	}

	memset(buf2, 0, sizeof(buf2));
	if (nvram_get_int("bt_log"))
		snprintf(buf2, sizeof(buf2), "-e %s/transmission.log", nvram_safe_get("bt_log_path"));

	memset(buf, 0, sizeof(buf));
#ifdef TCONFIG_STUBBY
	snprintf(buf, sizeof(buf), "EVENT_NOEPOLL=1; export EVENT_NOEPOLL; CURL_CA_BUNDLE=/etc/ssl/cert.pem; export CURL_CA_BUNDLE; %s/transmission-daemon -g %s/.settings %s", pn, pk, buf2);
#else
	snprintf(buf, sizeof(buf), "EVENT_NOEPOLL=1; export EVENT_NOEPOLL; %s/transmission-daemon -g %s/.settings %s", pn, pk, buf2);
#endif
	system(buf);

	sleep(1);
	if (pidof("transmission-da") > 0)
		logmsg(LOG_INFO, "transmission-daemon started");
	else
		logmsg(LOG_ERR, "starting transmission-daemon failed ...");

	sleep(2);
	eval("/usr/bin/btcheck", "addcru");

	pidof_child = 0; /* reset pid */

	/* terminate the child */
	exit(0);
}

void stop_bittorrent(void)
{
	pid_t pid;
	char buf[16];
	int n = 10;

	if (serialize_restart("transmission-da", 0))
		return;

	if (pidof("transmission-da") > 0) {
		logmsg(LOG_INFO, "Terminating transmission-daemon ...");

		killall_tk_period_wait("transmission-da", 50);
		sleep(1);
		while ((pid = pidof("transmission-da")) > 0 && (n-- > 0)) {
			logmsg(LOG_WARNING, "Killing transmission-daemon ...");
			/* Reap the zombie if it has terminated */
			waitpid(pid, NULL, WNOHANG);
			sleep(1);
		}

		if (n < 10)
			logmsg(LOG_WARNING, "transmission-daemon forcefully stopped");
		else
			logmsg(LOG_INFO, "transmission-daemon successfully stopped");
	}

	/* restore buffers */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%d", rmem_max);
	f_write_procsysnet("core/rmem_max", buf);
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%d", wmem_max);
	f_write_procsysnet("core/wmem_max", buf);

	eval("/usr/bin/btcheck", "addcru");
}
