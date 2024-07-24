include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS) -Wsign-compare

OBJS = udevtrigger.o

all: udevtrigger

%.o: %.c
	@echo " [udevtrigger] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

udevtrigger: $(OBJS)
	@echo " [udevtrigger] CC $@"
	@$(CC) $(CFLAGS) -o $@ $^
	$(SIZECHECK)
	$(CPTMP)

install: all
	@echo " [udevtrigger] Installing as $(INSTALLDIR)/sbin/udevtrigger"
	@install -D udevtrigger $(INSTALLDIR)/sbin/udevtrigger
	@$(STRIP) $(INSTALLDIR)/sbin/udevtrigger
	@chmod 0500 $(INSTALLDIR)/sbin/udevtrigger

clean:
	rm -f udevtrigger *.o
