# $Id: Makefile,v 1.2 2013/05/21 08:54:57 nanard Exp $
CC=gcc
CFLAGS=-Wall -g -I.

all:	testipfrdr

clean:
	rm *.o testipfrdr

testipfrdr:	testipfrdr.o ipfrdr.o
	$(CC) -o $@ $^

ipfrdr.o:	ipfrdr.c

testipfrdr.o:	testipfrdr.c

