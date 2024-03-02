include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections -Wsign-compare -I$(TOP)/shared
LDFLAGS = -fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections
LIBS    =

ifneq ($(TCONFIG_HTTPS),)
 ifeq ($(TCONFIG_OPENSSL11),y)
  CFLAGS += -I$(TOP)/openssl-1.1/include -DUSE_OPENSSL -DUSE_OPENSSL11
  LIBS   += -L$(TOP)/openssl-1.1 -lssl -lcrypto
 else
  CFLAGS += -I$(TOP)/openssl/include -DUSE_OPENSSL
  LIBS   += -L$(TOP)/openssl -lssl -lcrypto
 endif
else
 CFLAGS += -I$(TOP)/cyassl/include -I$(TOP)/cyassl/ctaocrypt/include
 LIBS   += $(TOP)/cyassl/src/.libs/libcyassl.a
endif

ifneq ($(TCONFIG_BCMARM),y)
 LIBS   += -lpthread
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
 export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = mssl.o

all: libmssl.so

libmssl.so: $(OBJS)
ifeq ($(TCONFIG_BCMARM),y)
	@echo " [mssl] LD $@"
	@$(LD) $(LIBS) -shared -o $@ $^
else
	@echo " [mssl] CC $@"
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)
endif
	$(SIZECHECK)
#	$(STRIP) $@
	$(CPTMP)

install: all
	@echo " [mssl] Installing as $(INSTALLDIR)/usr/lib/libmssl.so"
	@install -D libmssl.so $(INSTALLDIR)/usr/lib/libmssl.so
	@install -m 755 libmssl.so $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/libmssl.so

clean:
	rm -f *.o *.so
