/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"


void create_passwd(void)
{
	char s[512];
	char *p;
	char salt[32];
	FILE *f;
	mode_t m;
#ifdef TCONFIG_SAMBASRV
	char *smbd_user;
#endif

	strcpy(salt, "$1$");
	f_read("/dev/urandom", s, 6);
	base64_encode(s, salt + 3, 6);
	salt[3 + 8] = 0;
	p = salt;
	while (*p) {
		if (*p == '+')
			*p = '.';

		++p;
	}

	if ((p = nvram_safe_get("http_passwd")) && (*p == 0))
		p = "admin";

#ifdef TCONFIG_SAMBASRV
	smbd_user = nvram_safe_get("smbd_user");
	if ((*smbd_user == 0) || (!strcmp(smbd_user, "root")))
		smbd_user = "nas";
#endif

	m = umask(0777);
	if ((f = fopen("/etc/shadow", "w")) != NULL) {
		p = crypt(p, salt);
		fprintf(f, "root:%s:1:0:99999:7:0:0:\n"
			   "nobody:*:1:0:99999:7:0:0:\n", p);
#ifdef TCONFIG_SAMBASRV
		fprintf(f, "%s:*:1:0:99999:7:0:0:\n", smbd_user);
#endif

		fappend(f, "/etc/shadow.custom");
		fclose(f);
	}
	umask(m);
	chmod("/etc/shadow", 0600);

#ifdef TCONFIG_SAMBASRV
	memset(s, 0, 512);
	sprintf(s, "root:x:0:0:root:/root:/bin/sh\n"
	           "%s:x:100:100:nas:/dev/null:/dev/null\n"
	           "nobody:x:65534:65534:nobody:/dev/null:/dev/null\n",
	           smbd_user);

	f_write_string("/etc/passwd", s, 0, 0644);
#else
	f_write_string("/etc/passwd",
	               "root:x:0:0:root:/root:/bin/sh\n"
	               "nobody:x:65534:65534:nobody:/dev/null:/dev/null\n",
	               0, 0644);
#endif
	fappend_file("/etc/passwd", "/etc/passwd.custom");

	f_write_string("/etc/gshadow",
	               "root:*:0:\n"
#ifdef TCONFIG_SAMBASRV
	               "nas:*:100:\n"
#endif
	               "nobody:*:65534:\n",
	               0, 0600);

	fappend_file("/etc/gshadow", "/etc/gshadow.custom");

	f_write_string("/etc/group",
	               "root:x:0:\n"
#ifdef TCONFIG_SAMBASRV
	               "nas:x:100:\n"
#endif
	               "nobody:x:65534:\n",
	               0, 0644);

	fappend_file("/etc/group", "/etc/group.custom");
}

static inline int check_host_key(const char *ktype, const char *nvname, const char *hkfn)
{
	unlink(hkfn);

	if (!nvram_get_file(nvname, hkfn, 4096)) {
		eval("dropbearkey", "-t", (char *)ktype, "-f", (char *)hkfn);
		if (nvram_set_file(nvname, hkfn, 2048))
			return 1;
	}

	return 0;
}

void start_sshd(void)
{
	int dirty = 0;
	char *argv[11];
	int argc;
	char *p;

	mkdir("/etc/dropbear", 0700);
	mkdir("/root/.ssh", 0700);

	f_write_string("/root/.ssh/authorized_keys", nvram_safe_get("sshd_authkeys"), 0, 0600);

	dirty |= check_host_key("rsa",     "sshd_hostkey",  "/etc/dropbear/dropbear_rsa_host_key");
	dirty |= check_host_key("dss",     "sshd_dsskey",   "/etc/dropbear/dropbear_dss_host_key");
	dirty |= check_host_key("ecdsa",   "sshd_ecdsakey", "/etc/dropbear/dropbear_ecdsa_host_key");
	dirty |= check_host_key("ed25519", "sshd_ed25519",  "/etc/dropbear/dropbear_ed25519_host_key");
	if (dirty)
		nvram_commit_x();

	argv[0] = "dropbear";
	argv[1] = "-p";
	argv[2] = nvram_safe_get("sshd_port");
	argc = 3;

	if (nvram_get_int("sshd_remote") && nvram_invmatch("sshd_rport", nvram_safe_get("sshd_port"))) {
		argv[argc++] = "-p";
		argv[argc++] = nvram_safe_get("sshd_rport");
	}

	if (!nvram_get_int("sshd_pass"))
		argv[argc++] = "-s";

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	if (nvram_get_int("sshd_forwarding"))
		argv[argc++] = "-a";
#endif

	if ((p = nvram_safe_get("sshd_rwb")) && (*p)) {	/* only if sshd_rwb != "" */
		argv[argc++] = "-W";
		argv[argc++] = p;
	}

	argv[argc] = NULL;
	_eval(argv, NULL, 0, NULL);
}

void stop_sshd(void)
{
	killall("dropbear", SIGTERM);
}

void start_telnetd(void)
{
	xstart("telnetd", "-p", nvram_safe_get("telnetd_port"));
}

void stop_telnetd(void)
{
	killall("telnetd", SIGTERM);
}
