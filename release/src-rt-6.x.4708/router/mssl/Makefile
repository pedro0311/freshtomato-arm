include ../common.mak

CFLAGS  = -Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections -Wsign-compare
CFLAGS += -I$(TOP)/shared
LDFLAGS = -fPIC -ffunction-sections -fdata-sections --gc-sections -L$(TOP)/shared
LIBS    = -lshared -lpthread

ifneq ($(TCONFIG_HTTPS),)
 ifeq ($(TCONFIG_WOLFSSL),y)
  ifeq ($(TCONFIG_WOLFSSLMIN),y)
   ifneq ($(TCONFIG_OPENVPN),y)
    CFLAGS  += -DUSE_WOLFSSLMIN
   endif
  endif
  CFLAGS  += -DUSE_WOLFSSL -I$(TOP)/wolfssl/staged/usr/include -I$(TOP)/wolfssl/staged/usr/include/wolfssl -I$(TOP)/wolfssl
  LDFLAGS += -L$(TOP)/wolfssl/staged/usr/lib
  LIBS    += -lwolfssl
 else
  CFLAGS  += -DUSE_OPENSSL -I$(TOP)/$(OPENSSLDIR)/include
  LDFLAGS += -L$(TOP)/$(OPENSSLDIR)
  LIBS    += -lssl -lcrypto
 endif
else
 CFLAGS += -DUSE_CYASSL -I$(TOP)/cyassl/include -I$(TOP)/cyassl/ctaocrypt/include
 LIBS   += $(TOP)/cyassl/src/.libs/libcyassl.a
endif

ifeq ($(or $(TCONFIG_BCMARM),$(TCONFIG_BBT),$(TCONFIG_NGINX)),y)
 CFLAGS  += -DUSE_LIBCURL
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
 export CFLAGS += -DDEBUG_LOGMSG
endif

OBJS = mssl.o

all: libmssl.so

%.o: %.c
	@echo " [mssl] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

libmssl.so: $(OBJS)
	@echo " [mssl] LD $@"
	@$(LD) -shared -o $@ $^ $(LDFLAGS) $(LIBS)
	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo " [mssl] Installing as $(INSTALLDIR)/usr/lib/libmssl.so"
	@install -d $(INSTALLDIR)/usr/lib
	@install -m 755 libmssl.so $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/libmssl.so

clean:
	@rm -f *.o *.so
