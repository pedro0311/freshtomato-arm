include ../common.mak

all:

install:
#	for F in $(wildcard *); do \
#		if [ $$F = "Makefile" ]; then \
#			continue; \
#		fi; \
#		sed -i $$F -e "/^#!/p" -e "/^\s*#/d"; \
#		sed -i $$F -e '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^$$/d }'; \
#	done

	install -d $(INSTALLDIR)/sbin
	install -d $(INSTALLDIR)/usr/sbin
	install -d $(INSTALLDIR)/lib

ifeq ($(TCONFIG_USB),y)
# Optware perl symlink
	install -d $(INSTALLDIR)/usr/bin
	ln -sf /opt/bin/perl $(INSTALLDIR)/usr/bin/perl

#3G modem switch script
	install -m 0700 switch3g $(INSTALLDIR)/usr/sbin
#4G modem switch script
	install -m 0700 switch4g $(INSTALLDIR)/usr/sbin
#modem status script
	install -m 0700 wwansignal $(INSTALLDIR)/usr/sbin
endif

ifeq ($(TCONFIG_NGINX),y)
#MySQL
	install -D -m 755 mycheck $(INSTALLDIR)/usr/bin
endif

#MultiWAN watchdog script
	install -m 0700 watchdog $(INSTALLDIR)/usr/sbin

ifeq ($(TCONFIG_DNSCRYPT),y)
	install -m 0700 ntp2ip $(INSTALLDIR)/usr/sbin
endif

#TomatoAnon project
	install -m 0700 tomatoanon $(INSTALLDIR)/usr/sbin

#Adblock
	install -m 0700 adblock $(INSTALLDIR)/usr/sbin

#TTB project
	install -m 0700 ttb $(INSTALLDIR)/usr/sbin

#Ethernet state report
	install -m 0700 ethstate $(INSTALLDIR)/usr/sbin

#MOTD
	install -m 0700 mymotd $(INSTALLDIR)/usr/sbin

#sysinfo-helper
	install -m 0700 sysinfo-helper $(INSTALLDIR)/usr/sbin

#Link Aggregation
	install -m 0700 linkagg $(INSTALLDIR)/usr/sbin

#Web Monitor Backup Script
	install -m 0700 webmon_bkp $(INSTALLDIR)/usr/sbin

#optware/entware install script
ifeq ($(TCONFIG_USB),y)
ifneq ($(TCONFIG_BCMARM),y)
	install -m 0700 optware-install.sh $(INSTALLDIR)/usr/sbin
endif
	install -m 0700 entware-install.sh $(INSTALLDIR)/usr/sbin
endif

ifeq ($(TCONFIG_OPENVPN),y)
	install -m 0700 updown-client.sh $(INSTALLDIR)/usr/sbin
	install -m 0700 vpnrouting.sh $(INSTALLDIR)/usr/sbin
endif

# cron helper
	install -m 0700 cru $(INSTALLDIR)/usr/sbin

# system info
	install -m 0700 sysinfo $(INSTALLDIR)/usr/sbin

# BTGUI
ifeq ($(TCONFIG_BT),y)
	install -m 0755 btcheck $(INSTALLDIR)/usr/bin
endif

# network discovery script for status-devices page
ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_MIPSR2)),y)
	install -m 0755 discovery.sh $(INSTALLDIR)/usr/sbin
endif

clean:
