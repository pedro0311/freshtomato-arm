include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -Wsign-compare
CFLAGS += -I$(SRCBASE)/include -I$(TOP)/shared -ffunction-sections -fdata-sections
LDFLAGS = -ffunction-sections -fdata-sections -Wl,--gc-sections
LIBS    =

ifeq ($(TCONFIG_BCMARM),y)
 LIBS   += -lgcc_s
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
 export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = rstats.o

all: rstats

rstats: $(OBJS)
	@echo " [rstats] CC -o $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -L../shared -lshared -L../nvram${BCMEX} -lnvram $(LIBS)
	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f rstats *.o .*.depend

install: all
	@echo " [rstats] Installing as $(INSTALLDIR)/bin/rstats"
	@install -D rstats $(INSTALLDIR)/bin/rstats
	@$(STRIP) $(INSTALLDIR)/bin/rstats
	@chmod 0500 $(INSTALLDIR)/bin/rstats

%.o: %.c .%.depend
	@echo " [rstats] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
