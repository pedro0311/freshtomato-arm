#
# Linux bsd Makefile
#

include $(SRCBASE)/router/.config

CFLAGS	+= -I. -I$(TOP)/shared -I$(SRCBASE)/include -I$(SRCBASE)/common/include -Wall
CFLAGS	+= $(WLAN_ComponentIncPath)
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS	+= -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared
LDFLAGS += -lgcc_s

ifeq ($(TCONFIG_AMASDB),y)
CFLAGS += -DAMASDB
endif

ifeq ($(TCONFIG_BCMARM),y)
CFLAGS += -DTOMATO_ARM
endif

ifeq ($(TCONFIG_BCM7),y)
CFLAGS += ${WLAN_StdIncPathA}
endif

ifeq ($(TCONFIG_AC3200),y)
CFLAGS += -DTOMATO_CONFIG_AC3200
endif

vpath %.c $(SRCBASE)/shared $(WLAN_ComponentSrcDirs)

all: bsd

OBJS = bsd_main.o bsd.o bsd_engine.o  bsd_util.o bsd_comm.o bcmwifi_channels.o bsd_default.o

install: all
	install -d $(INSTALLDIR)/usr/sbin/
	install bsd $(INSTALLDIR)/usr/sbin/
	$(STRIP) $(INSTALLDIR)/usr/sbin/bsd

clean:
	rm -f *.o *.a *.so bsd

${OBJS}: bsd.h

bsd: bcmutils.o ${OBJS}
	$(CC) $(CFLAGS) $^ -o $@  $(LDFLAGS)

