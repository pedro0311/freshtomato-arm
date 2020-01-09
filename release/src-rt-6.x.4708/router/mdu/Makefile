include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared
LDFLAGS	=
LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared
#LIBS = -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared $(TOP)/mssl/libmssl.a $(TOP)/matrixssl/src/libmatrixsslstatic.a

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

ifeq ($(TCONFIG_OPENSSL11),y)
CFLAGS += -I$(TOP)/openssl-1.1/include
LIBS += -L$(TOP)/openssl-1.1 -lssl -lcrypto
else
CFLAGS += -I$(TOP)/openssl/include
LIBS += -L$(TOP)/openssl -lssl -lcrypto
endif

ifneq ($(TCONFIG_BBT)$(TCONFIG_NGINX),)
CFLAGS += -DUSE_LIBCURL -I$(TOP)/libcurl/staged/usr/include
LIBS += -lpthread -L$(TOP)/libcurl/lib/.libs -lcurl
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

