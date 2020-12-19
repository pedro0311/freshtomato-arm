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

#define OVPN_CLIENT_DIR		"/tmp/ovpnclientconfig"
#define OPENSSL_TMP_DIR		"/tmp/openssl"


#ifdef TCONFIG_KEYGEN
static void put_to_file(const char *filePath, const char *content)
{
	FILE *fkey = fopen(filePath, "w");
	fputs(content, fkey);
	fclose(fkey);
}

static void prepareCAGeneration(int serverNum)
{
	char buffer[512];
	char buffer2[512];
	eval("rm", "-Rf", OPENSSL_TMP_DIR);
	eval("mkdir", "-p", OPENSSL_TMP_DIR);
	put_to_file(OPENSSL_TMP_DIR"/index.txt", "");
	put_to_file(OPENSSL_TMP_DIR"/openssl.log", "");

	memset(buffer, 0, 512);
	sprintf(buffer, "vpn_server%d_ca_key", serverNum);

	if (nvram_match(buffer, "")) {
		syslog(LOG_WARNING, "No CA KEY was saved for server %d, regenerating", serverNum);
		memset(buffer2, 0, 512);
		sprintf(buffer2, "\"/C=GB/ST=Yorks/L=York/O=FreshTomato/OU=IT/CN=server.%s\"", nvram_safe_get("wan_domain"));
		memset(buffer, 0, 512);
		sprintf(buffer, "openssl req -days 3650 -nodes -new -x509 -keyout "OPENSSL_TMP_DIR"/cakey.pem -out "OPENSSL_TMP_DIR"/cacert.pem -subj %s >>"OPENSSL_TMP_DIR"/openssl.log 2>&1", buffer2);
		syslog(LOG_WARNING, buffer);
		system(buffer);
	}
	else {
		syslog(LOG_WARNING, "Found CA KEY for server %d, creating from NVRAM", serverNum);
		put_to_file(OPENSSL_TMP_DIR"/cakey.pem", getNVRAMVar("vpn_server%d_ca_key", serverNum));
		put_to_file(OPENSSL_TMP_DIR"/cacert.pem", getNVRAMVar("vpn_server%d_ca", serverNum));
	}
}

static void print_generated_ca_to_user()
{
	web_puts("cakey = '");
	web_putfile(OPENSSL_TMP_DIR"/cakey.pem", WOF_JAVASCRIPT);
	web_puts("';\ncacert = '");
	web_putfile(OPENSSL_TMP_DIR"/cacert.pem", WOF_JAVASCRIPT);
	web_puts("';");
}

static void generateKey(const char *prefix, const char *serial)
{
	char subj_buf[256];
	char buffer[512];
	char *str;

	if (strncmp(prefix, "server", 6) == 0) {
		str = "-extensions server_cert";
		syslog(LOG_WARNING, "Building Certs for Server");
	}
	else {
		str = "-extensions usr_cert";
		syslog(LOG_WARNING, "Building Certs for Client");
	}

	put_to_file(OPENSSL_TMP_DIR"/serial", serial);

	memset(subj_buf, 0, 256);
	sprintf(subj_buf, "\"/C=GB/ST=Yorks/L=York/O=FreshTomato/OU=IT/CN=%s.%s\"", prefix, nvram_safe_get("wan_domain"));
	memset(buffer, 0, 512);
	sprintf(buffer, "openssl req -nodes -new -keyout "OPENSSL_TMP_DIR"/%s.key -out "OPENSSL_TMP_DIR"/%s.csr %s -subj %s >>"OPENSSL_TMP_DIR"/openssl.log 2>&1", prefix, prefix, str, subj_buf);
	syslog(LOG_WARNING, buffer);
	system(buffer);

	memset(buffer, 0, 512);
	sprintf(buffer, "openssl ca -batch -policy policy_anything -days 3650 -out "OPENSSL_TMP_DIR"/%s.crt -in "OPENSSL_TMP_DIR"/%s.csr %s -subj %s >>"OPENSSL_TMP_DIR"/openssl.log 2>&1", prefix, prefix, str, subj_buf);
	syslog(LOG_WARNING, buffer);
	system(buffer);

	memset(buffer, 0, 512);
	sprintf(buffer, "openssl x509 -in "OPENSSL_TMP_DIR"/%s.crt -inform PEM -out "OPENSSL_TMP_DIR"/%s.crt -outform PEM >>"OPENSSL_TMP_DIR"/openssl.log 2>&1", prefix, prefix);
	syslog(LOG_WARNING, buffer);
	system(buffer);
}

static void print_generated_keys_to_user(const char *prefix)
{
	char buffer[32];

	web_puts("\ngenerated_crt = '");
	memset(buffer, 0, 32);
	sprintf(buffer, OPENSSL_TMP_DIR"/%s.crt", prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';\ngenerated_key = '");
	memset(buffer, 0, 32);
	sprintf(buffer, OPENSSL_TMP_DIR"/%s.key", prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';");
}
#endif /* TCONFIG_KEYGEN */
#endif /* TCONFIG_OPENVPN */

void wo_ovpn_status(char *url)
{
#ifdef TCONFIG_OPENVPN
	char buffer[256];
	char *type;
	char *str;
	int num = 0, pid;
	FILE *fp;

	type = 0;
	if ((str = webcgi_get("server")))
		type = "server";
	else if ((str = webcgi_get("client")))
		type = "client";

	num = str ? atoi(str) : 0;
	if ((type) && (num > 0)) {
		snprintf(buffer, sizeof(buffer), "vpn%s%d", type, num);
		if ((pid = pidof(buffer)) > 0) {
			/* Read the status file and repeat it verbatim to the caller */
			snprintf(buffer, sizeof(buffer), "/etc/openvpn/%s%d/status", type, num);

			/* Give it some time if it doesn't exist yet */
			if (!f_exists(buffer)) {
				sleep(5);
			}

			fp = fopen(buffer, "r");
			if (fp != NULL) {
				while (fgets(buffer, sizeof(buffer), fp) != NULL)
					web_puts(buffer);
			fclose(fp);
			}
		}
	}
#endif /* TCONFIG_OPENVPN */
}

void wo_ovpn_genkey(char *url)
{
#ifdef TCONFIG_OPENVPN
	char buffer[64];
	char *modeStr;
	char *serverStr;
	int server;

	strlcpy(buffer, webcgi_safeget("_mode", ""), sizeof(buffer));
	modeStr = js_string(buffer);	/* quicky scrub */
	if (modeStr == NULL) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		syslog(LOG_WARNING, "No mode was set to wo_vpn_genkey!");
#endif
		return;
	}

	strlcpy(buffer, webcgi_safeget("_server", ""), sizeof(buffer));
	serverStr = js_string(buffer);	/* quicky scrub */
	if (serverStr == NULL && ((!strncmp(modeStr, "static", 6)) || (!strcmp(modeStr, "key")))) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		syslog(LOG_WARNING, "No server was set to wo_vpn_genkey but it was required by mode!");
#endif
		return;
	}

	server = atoi(serverStr);

	if (!strcmp(modeStr, "static1"))	/* tls-auth / tls-crypt */
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		web_pipecmd("openvpn --genkey secret /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey | tail -n +4 && rm /tmp/genvpnkey", WOF_NONE);
	else if (!strcmp(modeStr, "static2"))	/* tls-crypt-v2 */
		web_pipecmd("openvpn --genkey tls-crypt-v2-server /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey && rm /tmp/genvpnkey", WOF_NONE);
#else
		web_pipecmd("openvpn --genkey --secret /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey | tail -n +4 && rm /tmp/genvpnkey", WOF_NONE);
#endif /* !TCONFIG_OPTIMIZE_SIZE_MORE */
#ifdef TCONFIG_KEYGEN
	else if (!strcmp(modeStr, "dh"))
		web_pipecmd("openssl dhparam -out /tmp/dh1024.pem 1024 >/dev/null 2>&1 && cat /tmp/dh1024.pem && rm /tmp/dh1024.pem", WOF_NONE);
	else {
		prepareCAGeneration(server);
		generateKey("server", "00");
		print_generated_ca_to_user();
		print_generated_keys_to_user("server");
	}
#endif /* TCONFIG_KEYGEN */
#endif /* TCONFIG_OPENVPN */
}

void wo_ovpn_genclientconfig(char *url)
{
#ifdef TCONFIG_OPENVPN
#ifdef TCONFIG_KEYGEN
	char buffer[256];
	char *serverStr;
	int server;
	int hmac;
	int tls = 0;
	int userauth, useronly;
	char *u;
	FILE *fp;

	strlcpy(buffer, webcgi_safeget("_server", ""), sizeof(buffer));
	serverStr = js_string(buffer);	/* quicky scrub */

	strlcpy(buffer, url, sizeof(buffer));
	u = js_string(buffer);

	if ((serverStr == NULL) || (u == NULL)) {
		syslog(LOG_WARNING, "No server '%s' for /%s", serverStr, u);
		return;
	}

	server = atoi(serverStr);

	userauth = atoi(getNVRAMVar("vpn_server%d_userpass", server));
	useronly = userauth && atoi(getNVRAMVar("vpn_server%d_nocert", server));

	eval("rm", "-Rf", OVPN_CLIENT_DIR);
	eval("mkdir", "-m", "0777", "-p", OVPN_CLIENT_DIR);

	fp = fopen(OVPN_CLIENT_DIR"/connection.ovpn", "w");

	memset(buffer, 0, 256);
	sprintf(buffer, "vpn_server%d_crypt", server);
	if (nvram_match(buffer, "tls"))
		tls = 1;

	/* Remote address */
	fprintf(fp, "# Config generated by FreshTomato %s, requires OpenVPN 2.4.0 or newer\n\n"
	            "remote %s %d\n",
	            nvram_safe_get("os_version"),
	            get_wanip("wan"),
	            atoi(getNVRAMVar("vpn_server%d_port", server)));

	/* Proto */
	strlcpy(buffer, getNVRAMVar("vpn_server%d_proto", server), sizeof(buffer));
	str_replace(buffer, "-server", "-client");
	fprintf(fp, "proto %s\n", buffer);

	/* Compression */
	strlcpy(buffer, getNVRAMVar("vpn_server%d_comp", server), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
		if ((!strcmp(buffer, "lz4")) || (!strcmp(buffer, "lz4-v2")))
			fprintf(fp, "compress %s\n", buffer);
		else if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n");	/* Disable, but can be overriden */
	}

	/* Interface */
	fprintf(fp, "dev %s\n", getNVRAMVar("vpn_server%d_if", server));

	/* Cipher */
	strlcpy(buffer, getNVRAMVar("vpn_server%d_ncp_ciphers", server), sizeof(buffer));
	if (tls == 1) {
		if (buffer[0] != '\0')
			fprintf(fp, "ncp-ciphers %s\n", buffer);
	}
	else {	/* secret */
		memset(buffer, 0, 256);
		sprintf(buffer, "vpn_server%d_cipher", server);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
	}

	/* Digest */
	memset(buffer, 0, 256);
	sprintf(buffer, "vpn_server%d_digest", server);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	if (tls == 1) {
		fprintf(fp, "client\n"
		            "remote-cert-tls server\n"
		            "ca ca.pem\n");

		if (!useronly)
			fprintf(fp, "cert client.crt\n"
			            "key client.key\n");

		memset(buffer, 0, 256);
		sprintf(buffer, "vpn_server%d_hmac", server);
		hmac = nvram_get_int(buffer);
		if (hmac >= 0) {
			if (hmac == 3)
				fprintf(fp, "tls-crypt static.key");
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			else if (hmac == 4)
				fprintf(fp, "tls-crypt-v2 static.key");
#endif /* TCONFIG_OPTIMIZE_SIZE_MORE */
			else {
				fprintf(fp, "tls-auth static.key");
				if (hmac == 0)
					fprintf(fp, " 1");
				else if (hmac == 1)
					fprintf(fp, " 0");
			}
			fprintf(fp, "\n");

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			if (hmac == 4) {
				/* tls-crypt-v2 */
				put_to_file(OVPN_CLIENT_DIR"/static-server.key", getNVRAMVar("vpn_server%d_static", server));
				system("openvpn --tls-crypt-v2 "OVPN_CLIENT_DIR"/static-server.key --genkey tls-crypt-v2-client "OVPN_CLIENT_DIR"/static.key >/dev/null 2>&1");
				eval("rm", OVPN_CLIENT_DIR"/static-server.key");
			}
			else
#endif /* TCONFIG_OPTIMIZE_SIZE_MORE */
			{
				/* tls-auth / tls-crypt */
				put_to_file(OVPN_CLIENT_DIR"/static.key", getNVRAMVar("vpn_server%d_static", server));
			}
		}

		put_to_file(OVPN_CLIENT_DIR"/ca.pem", getNVRAMVar("vpn_server%d_ca", server));

		if (!useronly) {
			prepareCAGeneration(server);
			generateKey("client", getNVRAMVar("vpn_server%d_serial", server));
			eval("cp", OPENSSL_TMP_DIR"/client.crt", OVPN_CLIENT_DIR);
			eval("cp", OPENSSL_TMP_DIR"/client.key", OVPN_CLIENT_DIR);
		}

		/* Auth */
		if (userauth)
			fprintf(fp, "auth-user-pass\n");
	}
	else {
		fprintf(fp, "mode p2p\n");
		memset(buffer, 0, 256);
		sprintf(buffer, "vpn_server%d_if", server);
		if (nvram_contains_word(buffer, "tap")) {
			fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_local", server));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_nm", server));
		}
		else {
			fprintf(fp, "ifconfig %s ", getNVRAMVar("vpn_server%d_remote", server));
			fprintf(fp, "%s\n", getNVRAMVar("vpn_server%d_local", server));
		}
		fprintf(fp, "secret static.key\n");
		put_to_file(OVPN_CLIENT_DIR"/static.key", getNVRAMVar("vpn_server%d_static", server));
	}

	fprintf(fp, "keepalive 15 60\n"
	            "resolv-retry infinite\n"
	            "nobind\n"
	            "float\n"
	            "verb 3\n"
	            "status status\n"
	            "; log /var/log/openvpn.log\n");

	fclose(fp);
	eval("tar", "-cf", OVPN_CLIENT_DIR".tar", "-C", OVPN_CLIENT_DIR, ".");
	do_file(OVPN_CLIENT_DIR".tar");
#endif /* TCONFIG_KEYGEN */
#endif /* TCONFIG_OPENVPN */
}
