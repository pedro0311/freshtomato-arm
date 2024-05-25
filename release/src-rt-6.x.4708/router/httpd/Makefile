include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -Wsign-compare
CFLAGS += -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/nvram$(BCMEX)
LDFLAGS = -ffunction-sections -fdata-sections -Wl,--gc-sections -L$(TOP)/shared -L$(TOP)/nvram$(BCMEX)
LIBS    = -lshared -lnvram $(EXTRALIBS)

ifeq ($(TCONFIG_BCMARM),y)
 CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
 CFLAGS += -I$(SRCBASE)/common/include
endif

ifneq ($(TCONFIG_HTTPS),)
 CFLAGS  += -I$(TOP)/$(OPENSSLDIR)/include
 LDFLAGS += -L$(TOP)/mssl -L$(TOP)/$(OPENSSLDIR)
 LIBS    += -lmssl -lssl -lcrypto
 ifeq ($(or $(TCONFIG_OPENSSL30),$(TCONFIG_OPENSSL11)),y)
  CFLAGS += -DUSE_OPENSSL11
  ifeq ($(TCONFIG_BCMARM),y)
   LIBS  += -lpthread
  endif
 endif
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
 export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = httpd.o cgi.o tomato.o version.o misc.o dhcp.o upgrade.o \
       traceping.o parser.o upnp.o ctnf.o nvram.o log.o webio.o \
       wl.o devlist.o ddns.o config.o bwm.o utf8.o cpuinfo.o \
       iptraffic.o #blackhole.o

ifeq ($(TCONFIG_USB),y)
 OBJS   += usb.o
 OBJS   += wwan.o
endif

ifeq ($(TCONFIG_NOCAT),y)
 OBJS   += nocat.o
endif

ifeq ($(TCONFIG_OPENVPN),y)
 OBJS   += openvpn.o
endif

ifeq ($(TCONFIG_PPTPD),y)
 OBJS   += pptpd.o
endif

ifeq ($(TCONFIG_WIREGUARD),y)
 OBJS   += wireguard.o
endif

ifeq ($(TCONFIG_IPERF),y)
 OBJS   += iperf.o
endif

all: httpd

%.o: %.c
	@echo " [httpd] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

httpd: $(OBJS)
	@echo " [httpd] CC $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
	@$(SIZECHECK)
	@$(CPTMP)

install:
	@echo " [httpd] Installing as $(INSTALLDIR)/usr/sbin/httpd"
	@install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	@$(STRIP) $(INSTALLDIR)/usr/sbin/httpd
	@chmod 0500 $(INSTALLDIR)/usr/sbin/httpd

clean:
	@rm -f httpd *.o
