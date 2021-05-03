/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"

#include <sys/reboot.h>
#include <wlutils.h>
#include <wlioctl.h>

//	#define DEBUG_TEST

static int gf;


static int get_btn(const char *name, uint32_t *bit, uint32_t *pushed)
{
	int gpio;
	int inv;
	
	if (nvget_gpio(name, &gpio, &inv)) {
		*bit = 1 << gpio;
		*pushed = inv ? 0 : *bit;
		return 1;
	}
	return 0;
}

int buttons_main(int argc, char *argv[])
{
	int gpio;
	int last;
	uint32_t mask;
	uint32_t ses_mask;
	uint32_t ses_pushed;
	uint32_t reset_mask;
	uint32_t reset_pushed;
	uint32_t wlan_mask;
	uint32_t wlan_pushed;
	int count;
	char s[16];
	char *p;
	int n;
	int ses_led;
	int model;

	/* get Router model */
	model = get_model();

	ses_mask = 0;
	ses_pushed = 0;
	wlan_mask = 0;
	wlan_pushed = 0;
	reset_pushed = 0;
	reset_mask = 1 << 4;
	ses_led = LED_DIAG;
	last = 0;

	switch (nvram_get_int("btn_override") ? MODEL_UNKNOWN : model) {
#ifdef CONFIG_BCMWL6A
	case MODEL_RTN18U:
		reset_mask = 1 << 7; /* reset button (active LOW) */
		ses_mask = 1 << 11; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC56U:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 15; /* wps button (active LOW) */
		wlan_mask = 1 << 7;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC67U:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC68U:
	case MODEL_RTAC68UV3:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC1900P:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC66U_B1:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_DIR868L:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed */
		break;
	case MODEL_AC15:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_AC18:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_F9K1113v2_20X0:
	case MODEL_F9K1113v2:
		reset_mask = 1 << 8; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed */
		break;
	case MODEL_WS880:
		reset_mask = 1 << 2; /* reset button (active LOW) */
		ses_mask = 1 << 3; /* wps button (active LOW) */
		wlan_mask = 1 << 15; /* power button (active LOW) --> use it to toggle wifi */
		ses_led = LED_AMBER; /* Use LED Amber (only Dummy) for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_EA6350v1:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6400:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6700:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6900:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_R1D:
		reset_mask = 1 << 17;
 		ses_led = LED_AOSS;
 		break;
	case MODEL_R6250:
	case MODEL_R6300v2:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 4; /* wps button (active LOW) */
		wlan_mask = 1 << 5;  /* wifi button (active LOW) */
		ses_led = LED_AOSS;
		break;
	case MODEL_R6400:
	case MODEL_R6400v2:
	case MODEL_R6700v3:
	case MODEL_XR300:
		reset_mask = 1 << 5; /* reset button (active LOW) */
		ses_mask = 1 << 3; /* wps button (active LOW) */
		wlan_mask = 1 << 4; /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_R6700v1:
	case MODEL_R7000:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 4; /* wps button (active LOW) */
		wlan_mask = 1 << 5;  /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
	case MODEL_WZR1750:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 12; /* wps button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. */
		break;
#endif /* CONFIG_BCMWL6A */
#ifdef CONFIG_BCM7
	case MODEL_RTAC3200:
		reset_mask = 1 << 11; /* reset button (active LOW) */
		ses_mask = 1 << 7; /* wps button (active LOW) */
		wlan_mask = 1 << 4;  /* wifi button (active LOW) */
		ses_led = LED_AOSS; /* Use LED AOSS for feedback if a button is pushed. */
		break;
	case MODEL_R8000:
		reset_mask = 1 << 6; /* reset button (active LOW) */
		ses_mask = 1 << 5; /* wps button (active LOW) */
		wlan_mask = 1 << 4; /* wifi button (active LOW) */
		ses_led = LED_DIAG; /* Use LED Diag for feedback if a button is pushed. Do not interfere with LED_AOSS --> used for WLAN SUMMARY LED */
		break;
#endif /* CONFIG_BCM7 */
	default:
		get_btn("btn_ses", &ses_mask, &ses_pushed);
		if (!get_btn("btn_reset", &reset_mask, &reset_pushed)) {
			return 1;
		}
		break;
	}
	mask = reset_mask | ses_mask | wlan_mask;

#ifdef DEBUG_TEST
	cprintf("reset_mask=0x%X reset_pushed=0x%X\n", reset_mask, reset_pushed);
	cprintf("wlan_mask=0x%X wlan_pushed=0x%X\n", wlan_mask, wlan_pushed);
	cprintf("ses_mask=0x%X\n", ses_mask);
	cprintf("ses_led=%d\n", ses_led);
#else
	if (fork() != 0) return 0;
	setsid();
#endif

	signal(SIGCHLD, chld_reap);

	if ((gf = gpio_open(mask)) < 0) return 1;

	while (1) {
		if (((gpio = _gpio_read(gf)) == ~0) || (last == (gpio &= mask)) || (check_action() != ACT_IDLE)) {
#ifdef DEBUG_TEST
			cprintf("gpio = %X\n", gpio);
#endif
			sleep(1);
			continue;
		}

		if ((gpio & reset_mask) == reset_pushed) {
#ifdef DEBUG_TEST
			cprintf("reset down\n");
#endif

			led(LED_DIAG, LED_OFF);

			count = 0;
			do {
				sleep(1);
				if (++count == 3) led(LED_DIAG, LED_ON);
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & reset_mask) == reset_pushed));

#ifdef DEBUG_TEST
			cprintf("reset count = %d\n", count);
#else
			if (count >= 3) {
				eval("mtd-erase2", "nvram");
				sync();
				reboot(RB_AUTOBOOT);
			}
			else {
				led(LED_DIAG, LED_ON);
				set_action(ACT_REBOOT);
				kill(1, SIGTERM);
			}
			exit(0);
#endif
		}

		if ((ses_mask) && ((gpio & ses_mask) == ses_pushed)) {
			count = 0;
			do {
				//	syslog(LOG_DEBUG, "ses-pushed: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, ses_pushed, ses_mask, count);

				led(ses_led, LED_ON);
				usleep(500000);
				led(ses_led, LED_OFF);
				usleep(500000);
				++count;
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & ses_mask) == ses_pushed));
			gpio &= mask;

			if ((ses_led == LED_DMZ) && (nvram_get_int("dmz_enable") > 0)) led(LED_DMZ, LED_ON); /* turn LED_DMZ back on if used for feedback */

			/* turn LED_AOSS (Power LED for Asus Router; WPS LED for Tenda Router AC15/AC18) back on if used for feedback (WPS Button); Check Startup LED setting (bit 2 used for LED_AOSS) */
			if ((ses_led == LED_AOSS) && (nvram_get_int("sesx_led") & 0x04) &&
			    ((model == MODEL_AC15) ||
			     (model == MODEL_AC18) ||
			     (model == MODEL_RTN18U) ||
			     (model == MODEL_RTAC56U) ||
			     (model == MODEL_RTAC66U_B1) ||
			     (model == MODEL_RTAC1900P) ||
			     (model == MODEL_RTAC67U) ||
			     (model == MODEL_RTAC68U) ||
			     (model == MODEL_RTAC68UV3) ||
			     (model == MODEL_RTAC3200))) led(ses_led, LED_ON);

			//	syslog(LOG_DEBUG, "ses-released: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, ses_pushed, ses_mask, count);
			syslog(LOG_INFO, "SES pushed. Count was %d.", count);

			if ((count != 3) && (count != 7) && (count != 11)) {
				n = count >> 2;
				if (n > 3) n = 3;
				/*
					0-2  = func0
					4-6  = func1
					8-10 = func2
					12+  = func3
				*/

#ifdef DEBUG_TEST
				cprintf("ses func=%d\n", n);
#else
				sprintf(s, "sesx_b%d", n);
				//	syslog(LOG_DEBUG, "ses-func: count=%d %s='%s'", count, s, nvram_safe_get(s));
				if ((p = nvram_get(s)) != NULL) {
					switch (*p) {
					case '1':	/* toggle wl */
						nvram_set("rrules_radio", "-1");
						eval("radio", "toggle");
						break;
					case '2':	/* reboot */
						kill(1, SIGTERM);
						break;
					case '3':	/* shutdown */
						kill(1, SIGQUIT);
						break;
					case '4':	/* run a script */
						sprintf(s, "%d", count);
						run_nvscript("sesx_script", s, 2);
						break;
#ifdef TCONFIG_USB
					case '5':	/* !!TB: unmount all USB drives */
						add_remove_usbhost("-2", 0);
						break;
#endif
					}
				}
#endif

			}
		}

		if ((wlan_mask) && ((gpio & wlan_mask) == wlan_pushed)) {
			count = 0;
			do {
				//	syslog(LOG_DEBUG, "wlan-pushed: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, wlan_pushed, wlan_mask, count);

				led(ses_led, LED_ON);
				usleep(500000);
				led(ses_led, LED_OFF);
				usleep(500000);
				++count;
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & wlan_mask) == wlan_pushed));
			gpio &= mask;

			/* turn LED_AOSS (Power LED for Asus Router; WPS LED for Tenda Router AC15/AC18) back on if used for feedback (WLAN Button); Check Startup LED setting (bit 2 used for LED_AOSS) */
			if ((ses_led == LED_AOSS) && (nvram_get_int("sesx_led") & 0x04) &&
			    ((model == MODEL_AC15) ||
			     (model == MODEL_AC18) ||
			     (model == MODEL_RTAC56U) ||
			     (model == MODEL_RTAC1900P) ||
			     (model == MODEL_RTAC68U) ||
			     (model == MODEL_RTAC68UV3) ||
			     (model == MODEL_RTAC3200))) led(ses_led, LED_ON);

			//	syslog(LOG_DEBUG, "wlan-released: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, wlan_pushed, wlan_mask, count);
			syslog(LOG_INFO, "WLAN pushed. Count was %d.", count);
			nvram_set("rrules_radio", "-1");
			eval("radio", "toggle");
		}

		last = gpio;
	}

	return 0;
}
