include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -Wsign-compare
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared -I$(TOP)/nvram$(BCMEX)
LDFLAGS = -ffunction-sections -fdata-sections -Wl,--gc-sections -L$(TOP)/shared -L$(TOP)/nvram$(BCMEX)
LIBS    = -lshared -lnvram $(EXTRALIBS)

OBJS = wanuptime.o

all: wanuptime

%.o: %.c
	@echo " [wanuptime] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

wanuptime: $(OBJS)
	@echo " [wanuptime] CC $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
	$(SIZECHECK)
	$(CPTMP)

install:
	@echo " [wanuptime] Installing as $(INSTALLDIR)/usr/sbin/wanuptime"
	@install -d $(INSTALLDIR)/usr/sbin
	@install -m 755 wanuptime $(INSTALLDIR)/usr/sbin/wanuptime
	$(STRIP) $(INSTALLDIR)/usr/sbin/wanuptime

clean:
	@rm -f wanuptime *.o
