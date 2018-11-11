/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"
#include "httpd.h"

#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <sys/statfs.h>
#include <netdb.h>
#include <net/route.h>

#ifdef TCONFIG_IPV6
#include <ifaddrs.h>
#endif

#include <wlioctl.h>
#include <wlutils.h>

#ifdef TCONFIG_OPENVPN

/* FIXME: Somehow, eval doesn't seem to work? */
static void run_program(const char *program)
{
	pclose(popen(program, "r"));
}

static void put_to_file(const char *filePath, const char *content)
{
	FILE *fkey = fopen(filePath, "w");
	fputs(content, fkey);
	fclose(fkey);
}

static char *getNVRAMVar(const char *text, const int serverNum)
{
	char buffer[32];
	sprintf(&buffer[0], text, serverNum);
	return nvram_safe_get(&buffer[0]);
}

static void prepareCAGeneration(int serverNum)
{
	char buffer[512];
	char buffer2[512];
	eval("rm", "-Rf", "/tmp/openssl");
	eval("mkdir", "-p", "/tmp/openssl");
	put_to_file("/tmp/openssl/index.txt", "");
	put_to_file("/tmp/openssl/openssl.log", "w");

	sprintf(&buffer[0], "vpn_server%d_ca_key", serverNum);

	if (nvram_match(buffer, "")) {
		syslog(LOG_WARNING, "No CA KEY was saved for server %d, regenerating", serverNum);
		sprintf(&buffer2[0], "\"/C=GB/ST=Yorks/L=York/O=Company/OU=IT/CN=server.%s\"", nvram_safe_get("wan_domain"));
		syslog(LOG_WARNING, "SUBJ is: %s", buffer2);
		sprintf(&buffer[0], "openssl req -days 3650 -nodes -new -x509 -keyout /tmp/openssl/cakey.pem -out /tmp/openssl/cacert.pem -subj %s >>/tmp/openssl/openssl.log 2>&1", buffer2);
		syslog(LOG_WARNING, buffer);
		run_program(buffer);
	} else {
		syslog(LOG_WARNING, "Found CA KEY for server %d, creating from NVRAM", serverNum);
		put_to_file("/tmp/openssl/cakey.pem", getNVRAMVar("vpn_server%d_ca_key", serverNum));
		put_to_file("/tmp/openssl/cacert.pem", getNVRAMVar("vpn_server%d_ca", serverNum));
	}
}

static void print_generated_ca_to_user()
{
	web_puts("cakey = '");
	web_putfile("/tmp/openssl/cakey.pem", WOF_JAVASCRIPT);
	web_puts("';\ncacert = '");
	web_putfile("/tmp/openssl/cacert.pem", WOF_JAVASCRIPT);
	web_puts("';");
}

static void generateKey(const char *prefix)
{
	char subj_buf[512];
	char buffer[512];
	put_to_file("/tmp/openssl/serial", "00");
	sprintf(&subj_buf[0], "\"/C=GB/ST=Yorks/L=York/O=Company/OU=IT/CN=%s.%s\"", prefix, nvram_safe_get("wan_domain"));
	syslog(LOG_WARNING, "SUBJ is: %s", subj_buf);
	sprintf(&buffer[0], "openssl req -days 3650 -nodes -new -keyout /tmp/openssl/%s.key -out /tmp/openssl/%s.csr -subj %s >>/tmp/openssl/openssl.log 2>&1", prefix, prefix, subj_buf);
	syslog(LOG_WARNING, buffer);
	run_program(buffer);

	sprintf(&buffer[0], "openssl ca -batch -policy policy_anything -days 3650 -out /tmp/openssl/%s.crt -in /tmp/openssl/%s.csr -subj %s >>/tmp/openssl/openssl.log 2>&1", prefix, prefix, subj_buf);
	syslog(LOG_WARNING, buffer);
	run_program(buffer);

	sprintf(&buffer[0], "openssl x509 -in /tmp/openssl/%s.crt -inform PEM -out /tmp/openssl/%s.crt -outform PEM >>/tmp/openssl/openssl.log 2>&1", prefix, prefix);
	syslog(LOG_WARNING, buffer);
	run_program(buffer);
}

static void print_generated_keys_to_user(const char *prefix)
{
	char buffer[72];
	web_puts("\ngenerated_crt = '");
	sprintf(&buffer[0],"/tmp/openssl/%s.crt", prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);
	web_puts("';\ngenerated_key = '");
	sprintf(&buffer[0],"/tmp/openssl/%s.key", prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);
	web_puts("';");
}
#endif

void wo_vpn_status(char *url)
{
#ifdef TCONFIG_OPENVPN
	char buf[256];
	char *type;
	char *str;
	int num;
	FILE *fp;

	type = 0;
	if ((str = webcgi_get("server")))
		type = "server";
	else if ((str = webcgi_get("client")))
		type = "client";

	num = str ? atoi(str) : 0;
	if (type && num > 0) {
		/* Trigger OpenVPN to update the status file */
		snprintf(&buf[0], sizeof(buf), "vpn%s%d", type, num);
		killall(&buf[0], SIGUSR2);

		/* Give it a chance to update the file */
		sleep(1);

		/* Read the status file and repeat it verbatim to the caller */
		snprintf(&buf[0], sizeof(buf), "/etc/openvpn/%s%d/status", type, num);
		fp = fopen(&buf[0], "r");
		if (fp != NULL) {
			while (fgets(&buf[0], sizeof(buf), fp) != NULL)
				web_puts(&buf[0]);
			fclose(fp);
		}
	}
#endif
}

void wo_vpn_genkey(char *url)
{
#ifdef TCONFIG_OPENVPN
	char buffer[72];
	char *modeStr;
	char *serverStr;
	int server;
	strlcpy(buffer, webcgi_safeget("_mode", ""), sizeof(buffer));
	modeStr = js_string(buffer);	/* quicky scrub */
	if (modeStr == NULL) {
		syslog(LOG_WARNING, "No mode was set to wo_vpn_genkey!");
		return;
	}
	if (!strcmp(modeStr, "static")) {
		web_pipecmd("openvpn --genkey --secret /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey | tail -n +4 && rm /tmp/genvpnkey", WOF_NONE);
	} else if (!strcmp(modeStr, "dh")) {
		web_pipecmd("openssl dhparam -out /tmp/dh1024.pem 1024 >/dev/null 2>&1 && cat /tmp/dh1024.pem && rm /tmp/dh1024.pem", WOF_NONE);
	} else if (!strcmp(modeStr, "stop")) {
		killall("openssl", SIGTERM);
	} else {
		strlcpy(buffer, webcgi_safeget("_server", ""), sizeof(buffer));
		serverStr = js_string(buffer);	/* quicky scrub */
		if (serverStr == NULL) {
			syslog(LOG_WARNING, "No server was set to wo_vpn_genkey but it was required by mode!");
			return;
		}
		server = atoi(serverStr);
		prepareCAGeneration(server);
		generateKey("client1");
		print_generated_ca_to_user();
		print_generated_keys_to_user("client1");
	}
#endif
}

void wo_vpn_genclientconfig(char *url)
{
#ifdef TCONFIG_OPENVPN
	char s[72];
	char s2[72];
	char *serverStr;
	int server;
	int hmac;
	char *u;
	FILE *fp;
	DIR *dp;

	strlcpy(s, webcgi_safeget("_server", ""), sizeof(s));
	serverStr = js_string(s);	/* quicky scrub */

	strlcpy(s, url, sizeof(s));
	u = js_string(s);

	if ((serverStr == NULL) || (u == NULL)) {
		syslog(LOG_WARNING, "No server '%s' for /%s", serverStr, u);
		return;
	}

	server = atoi(serverStr);

	if (!(dp = opendir("/tmp/ovpnclientconfig"))) {
		eval("mkdir", "-m", "0777", "-p", "/tmp/ovpnclientconfig");
	}
	closedir(dp);

	fp = fopen("/tmp/ovpnclientconfig/connection.ovpn", "w");
	fprintf(fp, "remote %s\n", get_wanip("wan"));

	fprintf(fp, "port %s\n", getNVRAMVar("vpn_server%d_port", server));

	fprintf(fp, "proto %s\n", getNVRAMVar("vpn_server%d_proto", server));

	strlcpy(s2, getNVRAMVar("vpn_server%d_comp", server), sizeof(s2));
	if (strcmp(s2, "-1")) {
		if (!strcmp(s2, "lz4") || !strcmp(s2, "lz4-v2")) {
			fprintf(fp, "compress %s\n", s2);
		} else if (!strcmp(s2, "yes")) {
			fprintf(fp, "compress lzo\n");
		} else if (!strcmp(s2, "adaptive")) {
			fprintf(fp, "comp-lzo adaptive\n");
		} else if (!strcmp(s2, "no")) {
			fprintf(fp, "compress\n");	/* Disable, but can be overriden */
		}
	}

	fprintf(fp, "dev %s\n", getNVRAMVar("vpn_server%d_if", server));
	if (nvram_contains_word(&s[0], "tap")) {
		fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_local", server));
		fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_nm", server));
	} else {
		fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_remote", server));
		fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_local", server));
	}

	sprintf(&s[0], "vpn_server%d_cipher", server);
	if (!nvram_contains_word(&s[0], "default"))
		fprintf(fp, "cipher %s\n", nvram_safe_get(&s[0]));

	sprintf(&s[0], "vpn_server%d_digest", server);
	if (!nvram_contains_word(&s[0], "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(&s[0]));

	sprintf(&s[0], "vpn_server%d_crypt", server);
	if (nvram_match(s, "tls")) {
		fprintf(fp, "client\n");
		fprintf(fp, "crypt tls\n");
		fprintf(fp, "ca ca.pem\n");
		put_to_file("/tmp/ovpnclientconfig/ca.pem", getNVRAMVar("vpn_server%d_ca", server));

		sprintf(&s[0], "vpn_server%d_hmac", server);
		hmac = nvram_get_int(&s[0]);
		if (hmac >= 0) {
			fprintf(fp, "tls-auth static.key\n");
			put_to_file("/tmp/ovpnclientconfig/static.key", getNVRAMVar("vpn_server%d_static", server));
		}

		fprintf(fp, "cert client.crt\n");
		fprintf(fp, "key client.key\n");
		prepareCAGeneration(server);
		generateKey("client");
		eval("cp", "/tmp/openssl/client.crt", "/tmp/ovpnclientconfig");
		eval("cp", "/tmp/openssl/client.key", "/tmp/ovpnclientconfig");
	} else {
		fprintf(fp, "secret static.key\n");
		put_to_file("/tmp/ovpnclientconfig/static.key", getNVRAMVar("vpn_server%d_static", server));
	}
	fclose(fp);
	eval("tar", "-cf", "/tmp/ovpnclientconfig.tar", "-C", "/tmp/ovpnclientconfig", ".");
	do_file("/tmp/ovpnclientconfig.tar");
#endif
}
