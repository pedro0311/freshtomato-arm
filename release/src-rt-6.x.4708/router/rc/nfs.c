/*
 * nfs.c
 *
 * Copyright (C) 2011 shibby
 *
 * changes, fixes 2020 pedro
 *
 */

#include <rc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#define NFS_EXPORT	"/etc/exports"


void start_nfs(void)
{
	struct stat st_buf;
	FILE *fp;
	char *buf;
	char *g, *p;
	char *dir, *address, *access, *sync, *subtree, *other;

	if (!nvram_match("nfs_enable", "1"))
		return;

	if ((pidof("nfsd") >= 0) && (pidof("mountd") >= 0) && (pidof("statd") >= 0)) {
		syslog(LOG_INFO, "NFS Server already running... stop");
		stop_nfs();
	}

	syslog(LOG_INFO, "Starting NFS Server...");

	/* create directories/files */
	mkdir_if_none("/var/lib");
	mkdir_if_none("/var/lib/nfs");
	mkdir_if_none("/var/lib/nfs/sm");
	mkdir_if_none("/var/lib/nfs/v4recovery");

	unlink("/var/lib/nfs/etab");
	unlink("/var/lib/nfs/xtab");
	unlink("/var/lib/nfs/rmtab");
	close(creat("/var/lib/nfs/etab", 0644));
	close(creat("/var/lib/nfs/xtab", 0644));
	close(creat("/var/lib/nfs/rmtab", 0644));

	if (stat(NFS_EXPORT, &st_buf) == 0)
		unlink(NFS_EXPORT);

	/* read exports from nvram */
	if ((buf = strdup(nvram_safe_get("nfs_exports"))) != NULL) {

		/* writing data to file */
		if ((fp = fopen(NFS_EXPORT, "w")) == NULL) {
			perror(NFS_EXPORT);
			return;
		}

		g = buf;

		/* dir < address < access < sync < subtree < other */
		while ((p = strsep(&g, ">")) != NULL) {
			if ((vstrsep(p, "<", &dir, &address, &access, &sync, &subtree, &other)) != 6)
				continue;

			fprintf(fp, "%s %s(%s,%s,%s,%s)\n", dir, address, access, sync, subtree, other);
		}
		free(buf);
		fclose(fp);
	}

	chmod(NFS_EXPORT, 0644);

	if (pidof("portmap") < 0)
		eval("/usr/sbin/portmap");

	eval("/usr/sbin/statd");

	if (nvram_match("nfs_enable_v2", "1")) {
#if defined(TCONFIG_BCMARM)
		eval("/usr/sbin/nfsd", "-V 2");
#else
		eval("/usr/sbin/nfsd");
#endif
		eval("/usr/sbin/mountd", "-V 2");
	}
	else {
		eval("/usr/sbin/nfsd", "-N 2");
		eval("/usr/sbin/mountd", "-N 2");
	}

	sleep(1);
	eval("/usr/sbin/exportfs", "-r");

	syslog(LOG_INFO, "NFS Server started");
}

void stop_nfs(void)
{
	syslog(LOG_INFO, "Stopping NFS Server...");

	eval("/usr/sbin/exportfs", "-ua");
	killall_tk("mountd");
	killall("nfsd", SIGKILL);
	killall_tk("statd");

	syslog(LOG_INFO, "NFS Server stopped");
}
