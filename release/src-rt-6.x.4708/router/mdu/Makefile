include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS) -Wsign-compare
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared
LDFLAGS	=
LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

ifneq ($(TCONFIG_HTTPS),)
CFLAGS += -I$(TOP)/$(OPENSSLDIR)/staged/usr/include
LIBS += -L$(TOP)/$(OPENSSLDIR)/staged/usr/lib -lssl -lcrypto
ifeq ($(TCONFIG_OPENSSL11),y)
LIBS += $(if $(TCONFIG_BCMARM),-lpthread,)
endif
endif

ifneq ($(TCONFIG_BBT)$(TCONFIG_NGINX),)
CFLAGS += -DUSE_LIBCURL -I$(TOP)/libcurl/staged/usr/include -I$(TOP)/zlib/staged/usr/include
LIBS += -L$(TOP)/libcurl/lib/.libs -lcurl -L$(TOP)/zlib/staged/usr/lib -lz -lpthread
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

clean:
	rm -f mdu .*.depend
	rm -f *.o

install: all
	@echo " [mdu] Installing..."
	install -D mdu $(INSTALLDIR)/bin/mdu
	$(STRIP) $(INSTALLDIR)/bin/mdu
	chmod 0500 $(INSTALLDIR)/bin/mdu

%.o: %.c .%.depend
	@echo " [mdu] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)

