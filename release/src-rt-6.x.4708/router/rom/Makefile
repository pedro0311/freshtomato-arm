include ../common.mak

all:
	@echo "" >rom/etc/motd
	@echo "" >>rom/etc/motd
	@echo "Tomato v"`cat ../shared/tomato_version`"" >>rom/etc/motd

clean:
	rm -f rom/etc/motd

install:
	mkdir -p $(INSTALLDIR)
	( tar --exclude=.svn --exclude=*.bak -cpf - rom ) | ( cd $(INSTALLDIR) && tar -xpf - )

	# remove comments, compress whitespace
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/services > $(INSTALLDIR)/rom/etc/services
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/protocols > $(INSTALLDIR)/rom/etc/protocols

ifeq ($(TCONFIG_HTTPS),y)
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/openssl.cnf > $(INSTALLDIR)/rom/etc/openssl.cnf
else
	rm -f $(INSTALLDIR)/rom/etc/openssl.cnf
endif

ifneq ($(TCONFIG_DNSSEC),y)
	rm -f $(INSTALLDIR)/rom/etc/trust-anchors.conf
endif

ifneq ($(TCONFIG_DNSCRYPT),y)
	rm -f $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv
else
	@count=0; \
	while [ $${count} -lt 6 ]; do \
	    wget https://raw.githubusercontent.com/dyne/dnscrypt-proxy/master/dnscrypt-resolvers.csv -O $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv; \
	    if [ $${?} -ne 0 ]; then \
		count=`expr $${count} + 1`; \
		if [ $${count} -gt 5 ]; then \
		    err_file="../../error_download_dnscrypt_resolvers"; \
		    ret=$$(date)": Dnscrypt resolvers download error, check your internet connection! Using recently downloaded file!"; \
		    echo "\n\n"$${ret}"\n\n"; \
		    echo $${ret} >> $${err_file}; \
		    cp rom/etc/dnscrypt-resolvers.csv $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv; \
		    if [ $${?} -ne 0 ]; then \
			ret=$$(date)": No local copy of the dnscrypt-resolvers.csv file has been found, aborting!"; \
			echo "\n\n"$${ret}"\n\n"; \
			echo $${ret} >> $${err_file}; \
			exit 1; \
		    fi; \
		fi; \
		sleep 2; \
	    else \
		count=6; \
	    fi; \
	done
endif

#	Used by Curl
	cp -f others/cacert.pem $(INSTALLDIR)/rom/cacert.pem
