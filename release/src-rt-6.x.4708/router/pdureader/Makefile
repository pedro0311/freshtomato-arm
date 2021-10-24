include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -D_GNU_SOURCE
LDFLAGS	=

OBJS = pdureader.o

all: pdureader

pdureader: $(OBJS)
	@echo " [pdureader] CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f pdureader .*.depend
	rm -f *.o

install: all
	@echo " [pdureader] Installing..."
	install -D pdureader $(INSTALLDIR)/bin/pdureader
	$(STRIP) $(INSTALLDIR)/bin/pdureader
	chmod 0500 $(INSTALLDIR)/bin/pdureader

%.o: %.c .%.depend
	@echo " [pdureader] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
