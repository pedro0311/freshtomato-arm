
#ifndef __PORTMAP_CONFIG_H
#define __PORTMAP_CONFIG_H

#ifndef PORTMAP_MAPPING_FILE
#define PORTMAP_MAPPING_FILE	"/var/run/portmap_mapping"
#endif

#ifndef PORTMAP_MAPPING_FMODE
#define PORTMAP_MAPPING_FMODE	0600
#endif

#ifndef LOG_PERROR
#define LOG_PERROR		0
#endif

#ifndef RPCUSER
#define RPCUSER			"bin"
#endif

#ifndef LOG_DAEMON
#define LOG_DAEMON		0
#endif

#ifndef DAEMON_UID
#define DAEMON_UID		1
#endif

#ifndef DAEMON_GID
#define DAEMON_GID		1
#endif

#endif
