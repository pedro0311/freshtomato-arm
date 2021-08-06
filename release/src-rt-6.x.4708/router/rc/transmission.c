/*
 * transmission.c
 *
 * Copyright (C) 2011 shibby
 *
 */


#include "rc.h"

#include <stdlib.h>
#include <shutils.h>
#include <utils.h>
#include <sys/stat.h>

#define TR_SETTINGS		"/tmp/settings.json"
#define TR_START_SCRIPT		"/tmp/start_transmission.sh"
#define TR_STOP_SCRIPT		"/tmp/stop_transmission.sh"


void start_bittorrent(void)
{
	FILE *fp;
	char *pb, *pc, *pd, *pe, *pf, *ph, *pi, *pj, *pk, *pl, *pm, *pn, *po, *pp, *pr, *pt, *pu;
	char *whitelistEnabled;

	/* make sure its really stop */
	stop_bittorrent();

	/* only if enabled... */
	if (!nvram_match("bt_enable", "1"))
		return;

	/* collecting data */
	if (nvram_match("bt_rpc_enable", "1"))   { pb = "true"; } else { pb = "false"; }
	if (nvram_match("bt_dl_enable", "1"))    { pc = "true"; } else { pc = "false"; }
	if (nvram_match("bt_ul_enable", "1"))    { pd = "true"; } else { pd = "false"; }
	if (nvram_match("bt_incomplete", "1"))   { pe = "true"; } else { pe = "false"; }
	if (nvram_match("bt_autoadd", "1"))      { pf = "true"; } else { pf = "false"; }
	if (nvram_match("bt_ratio_enable", "1")) { ph = "true"; } else { ph = "false"; }
	if (nvram_match("bt_dht", "1"))          { pi = "true"; } else { pi = "false"; }
	if (nvram_match("bt_pex", "1"))          { pj = "true"; } else { pj = "false"; }

	if      (nvram_match("bt_settings", "down_dir")) { pk = nvram_safe_get("bt_dir"); }
	else if (nvram_match("bt_settings", "custom"))   { pk = nvram_safe_get("bt_settings_custom"); }
	else                                             { pk = nvram_safe_get("bt_settings"); }

	if (nvram_match("bt_auth", "1")) {
		pl = "true";
		whitelistEnabled = "false";
	} else {
		pl = "false";
		whitelistEnabled = "true";
	}
	if (nvram_match("bt_blocklist", "1")) { pm = "true"; } else { pm = "false"; }

	if      (nvram_match("bt_binary", "internal")) { pn = "/usr/bin"; }
	else if (nvram_match("bt_binary", "optware") ) { pn = "/opt/bin"; }
	else                                           { pn = nvram_safe_get( "bt_binary_custom"); }

	if (nvram_match("bt_lpd", "1"))               { po = "true"; } else { po = "false"; }
	if (nvram_match("bt_utp", "1"))               { pp = "true"; } else { pp = "false"; }
	if (nvram_match("bt_ratio_idle_enable", "1")) { pr = "true"; } else { pr = "false"; }
	if (nvram_match("bt_dl_queue_enable", "1"))   { pt = "true"; } else { pt = "false"; }
	if (nvram_match("bt_ul_queue_enable", "1"))   { pu = "true"; } else { pu = "false"; }

	/* writing data to file */
	if (!(fp = fopen(TR_SETTINGS, "w"))) {
		perror(TR_SETTINGS);
		return;
	}

	fprintf(fp, "{\n"
	            "\"peer-port\": %s, \n"
	            "\"speed-limit-down-enabled\": %s, \n"
	            "\"speed-limit-up-enabled\": %s, \n"
	            "\"speed-limit-down\": %s, \n"
	            "\"speed-limit-up\": %s, \n"
	            "\"rpc-enabled\": %s, \n"
	            "\"rpc-bind-address\": \"0.0.0.0\", \n"
	            "\"rpc-port\": %s, \n"
	            "\"rpc-whitelist\": \"*\", \n"
	            "\"rpc-whitelist-enabled\": %s, \n"
	            "\"rpc-host-whitelist\": \"*\", \n"
	            "\"rpc-host-whitelist-enabled\": %s, \n"
	            "\"rpc-username\": \"%s\", \n"
	            "\"rpc-password\": \"%s\", \n"
	            "\"download-dir\": \"%s\", \n"
	            "\"incomplete-dir-enabled\": \"%s\", \n"
	            "\"incomplete-dir\": \"%s/.incomplete\", \n"
	            "\"watch-dir\": \"%s\", \n"
	            "\"watch-dir-enabled\": %s, \n"
	            "\"peer-limit-global\": %s, \n"
	            "\"peer-limit-per-torrent\": %s, \n"
	            "\"upload-slots-per-torrent\": %s, \n"
	            "\"dht-enabled\": %s, \n"
	            "\"pex-enabled\": %s, \n"
	            "\"lpd-enabled\": %s, \n"
	            "\"utp-enabled\": %s, \n"
	            "\"ratio-limit-enabled\": %s, \n"
	            "\"ratio-limit\": %s, \n"
	            "\"idle-seeding-limit-enabled\": %s, \n"
	            "\"idle-seeding-limit\": %s, \n"
	            "\"blocklist-enabled\": %s, \n"
	            "\"blocklist-url\": \"%s\", \n"
	            "\"download-queue-enabled\": %s, \n"
	            "\"download-queue-size\": %s, \n"
	            "\"seed-queue-enabled\": %s, \n"
	            "\"seed-queue-size\": %s, \n"
	            "\"message-level\": %s, \n"
	            "%s%s"
	            "\"rpc-authentication-required\": %s \n"
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
	            (strcmp(nvram_safe_get("bt_custom"), "") ? ", \n" : ""),
	            pl);

	fclose(fp);

	chmod(TR_SETTINGS, 0644);

	/* start script */
	if (!(fp = fopen(TR_START_SCRIPT, "w"))) {
		perror(TR_START_SCRIPT);
		return;
	}

	fprintf(fp, "#!/bin/sh\n"
	            "sleep %s\n",
	            nvram_safe_get("bt_sleep"));

	if (nvram_match("bt_incomplete", "1"))
		fprintf(fp, "[ ! -d \"%s/.incomplete\" ] && {\n"
		            "mkdir %s/.incomplete\n"
		            "}\n",
		            pk,
		            pk);

	fprintf(fp, "[ ! -d \"%s/.settings\" ] && {\n"
	            "mkdir %s/.settings\n"
	            "}\n"
	            /* backward options compatibility (worked only with trailing comma before)
	             * TBD: need better regex, trim triple commas, be safe for passwords etc
	             */
	            "sed -i 's/,,\\s/, /g' "TR_SETTINGS"\n"
	            "mv "TR_SETTINGS" %s/.settings\n"
	            "rm %s/.settings/blocklists/*\n",
	            pk,
	            pk,
	            pk,
	            pk);

	if (nvram_match( "bt_blocklist", "1"))
		fprintf(fp, "wget %s -O %s/.settings/blocklists/level1.gz\n"
		            "gunzip %s/.settings/blocklists/level1.gz\n",
		            nvram_safe_get("bt_blocklist_url"), pk,
		            pk);

	fprintf(fp, "echo 4194304 > /proc/sys/net/core/rmem_max\n"		/* tune buffers */
	            "echo 2080768 > /proc/sys/net/core/wmem_max\n"
	            "EVENT_NOEPOLL=1; export EVENT_NOEPOLL\n"			/* crash fix? */
	            "CURL_CA_BUNDLE=/etc/ssl/cert.pem; export CURL_CA_BUNDLE\n"	/* workaround for missing cacert (in new curl versions) */
	            "%s/transmission-daemon -g %s/.settings",
	            pn,
	            pk);

	if (nvram_match("bt_log", "1"))
		fprintf(fp, " -e %s/transmission.log", nvram_safe_get("bt_log_path"));

	fprintf(fp, "\n"
	            "logger \"Transmission daemon started\" \n"
	            "sleep 2\n"
	            "/usr/bin/btcheck addcru\n");

	fclose(fp);

	chmod(TR_START_SCRIPT, 0755);

	xstart(TR_START_SCRIPT);

}

void stop_bittorrent(void)
{
	FILE *fp;

	if (pidof("transmission-da") > 0) {
		/* stop script */
		if (!(fp = fopen(TR_STOP_SCRIPT, "w"))) {
			perror(TR_STOP_SCRIPT);
			return;
		}

		fprintf(fp, "#!/bin/sh\n"
		            "COUNT=0\n"
		            "TIMEOUT=10\n"
		            "SLEEP=1\n"
		            "logger \"Terminating transmission-daemon...\" \n"
		            "killall transmission-daemon\n"
		            "while [ $(pidof transmission-daemon | awk '{print $1}') ]; do\n"
		            "sleep $SLEEP\n"
		            "COUNT=$(($COUNT + $SLEEP))\n"
		            "[ \"$COUNT\" -ge \"$TIMEOUT\" ] && {\n"
		            "logger \"Killing transmission-daemon...\" \n"
		            "killall -KILL transmission-daemon\n"
		            "}\n"
		            "done\n"
		            "[ \"$COUNT\" -lt \"$TIMEOUT\" ] && {\n"
		            "logger \"Transmission daemon successfully stopped\" \n"
		            "} || {\n"
		            "logger \"Transmission daemon forcefully stopped\" \n"
		            "}\n"
		            "/usr/bin/btcheck addcru\n"
		            "echo 1040384 > /proc/sys/net/core/rmem_max\n" /* tune-back buffers */
		            "echo 1040384 > /proc/sys/net/core/wmem_max\n"
		            "exit 0\n");

		fclose(fp);

		chmod(TR_STOP_SCRIPT, 0755);

		xstart(TR_STOP_SCRIPT);
	}
}
