#
# $Id: Makefile 382663 2013-02-03 04:40:03Z $
#

include ../../common.mak

CROSS =
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

ALL_OBJS = igsu.o igsu_linux.o

CCFLAGS = -Os -Wall -I$(SRCBASE)/include -I$(SRCBASE)/common/include -I$(SRCBASE)/shared -I$(SRCBASE)/include/emf/igs
CCFLAGS	+= $(WLAN_ComponentIncPath)

ifeq ($(wildcard $(SRCBASE)/router/emf$(BCMEX)/igsconf/*.c),)
ifeq ($(TCONFIG_BCMSMP),y) #for dual-core
all:
	-cp prebuilt/igs igs
else #for single-core
all:
	-cp prebuilt/up/igs igs
endif
else
all: igs
endif

igs: $(ALL_OBJS)
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ $(ALL_OBJS) $(LDLIBS)
	$(STRIP) $@

$(ALL_OBJS): %.o: %.c
	@echo " "
	@echo "Making $<"
	$(CC) $(CCFLAGS) -c $<

install: all
	install -d $(INSTALLDIR)/igs$(BCMEX)/usr/sbin
	install -m 755 igs $(INSTALLDIR)/igs$(BCMEX)/usr/sbin

.PHONY:	clean
clean:
	-rm -f *.o *~ core igs
