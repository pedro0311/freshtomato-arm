/*
 * ups.c
 *
 * Copyright (C) 2011 shibby
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define APCUPSD_DATA	"/www/ext/cgi-bin/tomatodata.cgi"
#define APCUPSD_UPS	"/www/ext/cgi-bin/tomatoups.cgi"


void start_ups(void)
{
/*  always copy and try start service if USB support is enable
 *  if service will not find apc ups, then will turn off automaticaly
 */
	eval("cp", "/www/apcupsd/tomatodata.cgi", APCUPSD_DATA);
	eval("cp", "/www/apcupsd/tomatoups.cgi", APCUPSD_UPS);

	xstart("apcupsd");
}

void stop_ups(void)
{
	killall("apcupsd", SIGTERM);
	eval("rm", APCUPSD_DATA);
	eval("rm", APCUPSD_UPS);
}
