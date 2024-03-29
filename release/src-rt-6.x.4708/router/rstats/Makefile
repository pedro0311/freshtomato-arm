include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -Wsign-compare
CFLAGS += -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/nvram$(BCMEX)
LDFLAGS = -ffunction-sections -fdata-sections -Wl,--gc-sections -L$(TOP)/shared -L$(TOP)/nvram$(BCMEX)
LIBS    = -lshared -lnvram $(EXTRALIBS)

ifeq ($(TOMATO_EXPERIMENTAL),1)
 export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = rstats.o

all: rstats

rstats: $(OBJS)
	@echo " [rstats] CC -o $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo " [rstats] Installing as $(INSTALLDIR)/bin/rstats"
	@install -D rstats $(INSTALLDIR)/bin/rstats
	@$(STRIP) $(INSTALLDIR)/bin/rstats
	@chmod 0500 $(INSTALLDIR)/bin/rstats

clean:
	@rm -f rstats *.o
