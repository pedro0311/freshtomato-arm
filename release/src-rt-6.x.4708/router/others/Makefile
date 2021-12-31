include ../common.mak

all:

install:
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
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/tomatoanon -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/tomatoanon -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif

#Adblock
	install -m 0700 adblock $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/adblock -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif

#TTB project
	install -m 0700 ttb $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/ttb -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/ttb -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif

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
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/optware-install.sh -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif
endif
	install -m 0700 entware-install.sh $(INSTALLDIR)/usr/sbin
ifneq ($(TCONFIG_STUBBY),y)
	sed -i $(INSTALLDIR)/usr/sbin/entware-install.sh -e "/STUBBY-BEGIN/,/STUBBY-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/entware-install.sh -e "/STUBBYNO-BEGIN/,/STUBBYNO-END/d"
endif
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

# certificate generator
ifneq ($(TCONFIG_HTTPS),)
	install -m 0500 gencert.sh $(INSTALLDIR)/usr/sbin
endif

# distinguish version if ARM or MIPS
ifeq ($(TCONFIG_OPENVPN),y)
ifneq ($(TCONFIG_BCMARM),y)
	sed -i $(INSTALLDIR)/usr/sbin/vpnrouting.sh -e "/BCMARM-BEGIN/,/BCMARM-END/d"
else
	sed -i $(INSTALLDIR)/usr/sbin/vpnrouting.sh -e "/BCMARMNO-BEGIN/,/BCMARMNO-END/d"
endif
endif

# clean up
	cd $(INSTALLDIR)/usr/sbin && \
		for F in tomatoanon adblock ttb optware-install.sh entware-install.sh vpnrouting.sh; do \
			[ -f $(INSTALLDIR)/usr/sbin/$$F ] && sed -i $$F \
			-e "/STUBBY-BEGIN/d"	-e "/STUBBY-END/d" \
			-e "/STUBBYNO-BEGIN/d"	-e "/STUBBYNO-END/d" \
			-e "/BCMARM-BEGIN/d"	-e "/BCMARM-END/d" \
			-e "/BCMARMNO-BEGIN/d"	-e "/BCMARMNO-END/d" \
			|| true; \
		done

ifneq ($(TCONFIG_BCMARM),y)
	cd $(INSTALLDIR)/usr/sbin && \
		for F in switch3g switch4g wwansignal watchdog ntp2ip tomatoanon adblock ttb ethstate mymotd sysinfo-helper linkagg webmon_bkp optware-install.sh entware-install.sh updown-client.sh vpnrouting.sh cru sysinfo stealthMode gencert.sh; do \
			[ -f $(INSTALLDIR)/usr/sbin/$$F ] && { \
				sed -i $$F -e "/^#!/p" -e "/^\s*#/d"; \
				sed -i $$F -e '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^$$/d }'; \
			} || true; \
	done
ifeq ($(or $(TCONFIG_BT),$(TCONFIG_NGINX)),y)
	cd $(INSTALLDIR)/usr/bin && \
		for F in mycheck btcheck; do \
			[ -f $(INSTALLDIR)/usr/bin/$$F ] && { \
				sed -i $$F -e "/^#!/p" -e "/^\s*#/d"; \
				sed -i $$F -e '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^$$/d }'; \
			} || true; \
	done
endif
endif

clean:
