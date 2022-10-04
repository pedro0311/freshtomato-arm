include ../router/.config

CC = gcc
CFLAGS = -O3 -fno-delete-null-pointer-checks -Wall
ifeq ($(TCONFIG_BCMWL6A),y)
CFLAGS += -DBCMWL6A
endif
ifeq ($(TCONFIG_TRXKEY),y)
CFLAGS += -DASUSKEY
endif

all: trx_asus

trx_asus: trx_asus.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o trx_asus piggy target.squashfs vmlinuz-lzma

%.o: %.c
	$(CC) $(CFLAGS) -c $<
