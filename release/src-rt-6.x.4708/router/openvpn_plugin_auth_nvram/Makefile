include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS) -fPIC
CFLAGS += -I$(SRCBASE)/include -I$(TOP)/$(OPENSSLDIR)/include $(if $(TCONFIG_BCMARM),-I$(TOP)/openvpn/include,-I$(TOP)/openvpn-2.5/include)
LDFLAGS = -L$(TOP)/nvram$(BCMEX) -lnvram -L$(TOP)/$(OPENSSLDIR) $(if $(TCONFIG_BCMARM),-L$(TOP)/openvpn,-L$(TOP)/openvpn-2.5)


ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

OBJS = openvpn_plugin_auth_nvram.o

all: openvpn_plugin_auth_nvram.so

openvpn_plugin_auth_nvram.so: $(OBJS)
	@echo " [openvpn_plugin_auth_nvram] CC -o $@"
	@$(CC) -shared $(LDFLAGS) -o $@ $(OBJS) $(LDFLAGS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D openvpn_plugin_auth_nvram.so $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	$(STRIP) $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	chmod 0500 $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so

clean:
	rm -f openvpn_plugin_auth_nvram.so .*.depend *.o
	
%.o: %.c .%.depend
	@echo " [openvpn_plugin_auth_nvram] CC $@"
	@$(CC) $(CFLAGS) -c $<
	
.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
