include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS) -Wsign-compare
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared
LDFLAGS	=
LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
export CFLAGS += -DDEBUG_LOGMSG
endif

ifneq ($(TCONFIG_HTTPS),)
CFLAGS += -I$(TOP)/$(OPENSSLDIR)/staged/usr/include
LIBS += -L$(TOP)/$(OPENSSLDIR)/staged/usr/lib -lssl -lcrypto
ifeq ($(TCONFIG_OPENSSL11),y)
LIBS += $(if $(TCONFIG_BCMARM),-lpthread,)
endif
endif

ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_BBT),$(TCONFIG_NGINX)),y)
CFLAGS += -DUSE_LIBCURL -I$(TOP)/zlib/staged/usr/include -I$(TOP)/libcurl/staged/usr/include
LIBS += -L$(TOP)/zlib/staged/usr/lib -lz -L$(TOP)/libcurl/lib/.libs -lcurl -lpthread
else
CFLAGS+= -I$(TOP)/mssl
LIBS += -L$(TOP)/mssl -lmssl
endif

OBJS = mdu.o

all: mdu

mdu: $(OBJS)
	@echo " [mdu] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo " [mdu] Installing..."
	install -D mdu $(INSTALLDIR)/bin/mdu
	$(STRIP) $(INSTALLDIR)/bin/mdu
	chmod 0500 $(INSTALLDIR)/bin/mdu

clean:
	rm -f mdu *.o .*.depend

%.o: %.c .%.depend
	@echo " [mdu] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
