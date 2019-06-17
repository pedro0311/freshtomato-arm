/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

__attribute__ ((noreturn))
static void help(void)
{
	char s[256];
	int i;

	s[0] = 0;
	for (i = 0; i < LED_COUNT; ++i) {
		if (led(i, LED_PROBE)) {
			if (s[0]) strcat(s, "/");
			strcat(s, led_names[i]);
		}
	}
	if (nvram_match("stealth_mode", "1")) { /* stealth mode ON ? */
		fprintf(stderr, "Stealth mode is turned ON. Please disable stealth mode if you want to use led command.\n");
	}
	else if (s[0] == 0) {
		fprintf(stderr, "Not supported.\n");
	}
	else {
		fprintf(stderr, "led <%s> <on/off> [...]\n", s);
	}
	exit(1);
}

int led_main(int argc, char *argv[])
{

	int i;
	int j;
	char *a;

	if ((argc < 3) || ((argc % 2) != 1)) help();

	for (j = 1; j < argc; j += 2) {
		a = argv[j]; /* led name */
		for (i = 0; i < LED_COUNT; ++i) {
			if (strcmp(led_names[i], a) == 0) /* full led name (usb/usb3 workaround) */
				break;
		}
		a = argv[j + 1]; /* action (on/off) */
		if ((i >= LED_COUNT) || ((strcmp(a, "on") != 0) && (strcmp(a, "off") != 0))) help();

		if (((i == LED_WLAN) || (i == LED_5G) || (i == LED_52G)) && nvram_get_int("blink_wl")) { /* For WLAN LEDs with blink turned on; If TRUE, stop/kill blink first */
			if (led(i, LED_PROBE)) { /* check for GPIO and non GPIO */
				killall("blink", SIGKILL); /* kill all blink */
				usleep(50000); /* wait 50 ms */
				led(i, (a[1] == 'n')); /* turn LED on/off */
			}
			else {
				help();
			}
		}
		else if (i == LED_BRIDGE) { /* For BRIDGE LED(s) */
			if (led(i, LED_PROBE)) { /* check for GPIO and non GPIO */
				killall("blink_br", SIGKILL); /* kill all blink_br */
				usleep(50000); /* wait 50 ms */

				if (a[1] == 'n') { /* case LED on */
					led(i, LED_ON); /* turn BRIDGE LED(s) on */
					eval("blink_br"); /* and also start blink_br again */
				}
				else {
					led(i, LED_OFF); /* turn BRIDGE LED(s) off */
				}
			}
			else {
				help();
			}
		}
		else if (!led(i, (a[1] == 'n'))) help(); /* default case */
	}

	return 0;
}
