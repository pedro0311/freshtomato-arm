/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bcmnvram.h>
#include <sys/ioctl.h>
#include <linux_gpio.h>

#include "utils.h"
#include "shutils.h"
#include "shared.h"


const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g", "52g"};


static int _gpio_ioctl(int f, int gpioreg, unsigned int mask, unsigned int val)
{
	struct gpio_ioctl gpio;

	gpio.val = val;
	gpio.mask = mask;

	if (ioctl(f, gpioreg, &gpio) < 0) {
		_dprintf("Invalid gpioreg %d\n", gpioreg);
		return -1;
	}
	return (gpio.val);
}

static int _gpio_open()
{
	int f = open("/dev/gpio", O_RDWR);
	if (f < 0)
		_dprintf ("Failed to open /dev/gpio\n");
	return f;
}

int gpio_open(uint32_t mask)
{
	uint32_t bit = 0;
	int i = 0 ;
	int f = _gpio_open();

	if ((f >= 0) && mask) {
		for (i = TOMATO_GPIO_MIN; i <= TOMATO_GPIO_MAX; i++) {
			bit = 1 << i;
			if ((mask & bit) == bit) {
				_gpio_ioctl(f, GPIO_IOC_RESERVE, bit, bit);
				_gpio_ioctl(f, GPIO_IOC_OUTEN, bit, 0);
			}
		}
		close(f);
		f = _gpio_open();
	}
	return f;
}

void gpio_write(uint32_t bit, int en)
{
	int f;

	if ((f = gpio_open(0)) < 0) return;

	_gpio_ioctl(f, GPIO_IOC_RESERVE, bit, bit);
	_gpio_ioctl(f, GPIO_IOC_OUTEN, bit, bit);
	_gpio_ioctl(f, GPIO_IOC_OUT, bit, en ? bit : 0);
	close(f);
}

uint32_t _gpio_read(int f)
{
	uint32_t r;
	r = _gpio_ioctl(f, GPIO_IOC_IN, 0x07FF, 0);
	if (r < 0) r = ~0;
	return r;
}

uint32_t gpio_read(void)
{
	int f;
	uint32_t r;

	if ((f = gpio_open(0)) < 0) return ~0;
	r = _gpio_read(f);
	close(f);
	return r;
}

int nvget_gpio(const char *name, int *gpio, int *inv)
{
	char *p;
	uint32_t n;

	if (((p = nvram_get(name)) != NULL) && (*p)) {
		n = strtoul(p, NULL, 0);
		if ((n & 0xFFFFFF60) == 0) {		/* bin 0110 0000 */
			*gpio = (n & TOMATO_GPIO_MAX);	/* bin 0001 1111 */
			*inv = ((n & 0x80) != 0);	/* bin 1000 0000 */
			/* 0x60 + 0x1F (dec 31) + 0x80 = 0xFF */
			return 1;
		}
	}
	return 0;
}

int do_led(int which, int mode)
{
  /*
   * valid GPIO values: 0 to 31 (default active LOW, inverted or active HIGH with -[value])
   * value 255: not known / disabled / not possible
   * value -99: special case for -0 substitute (active HIGH for GPIO 0)
   * value 254: non GPIO LED (special case, to show there is something!)
   */
//				   WLAN  DIAG  WHITE AMBER   DMZ  AOSS  BRIDGE USB2 USB3    5G   52G
//				   ----  ----  ----- -----   ---  ----  ------ ---- ----    --   ---
	static int wrt54g[]	= { 255,    1,     2,    3,    7,  255,  255,  255,  255,  255,  255 };
	static int wrtsl[]	= { 255,    1,     5,    7,    0,  255,  255,  255,  255,  255,  255 };
	static int whrg54[]	= {   2,    7,   255,  255,  255,    6,    1,    3,  255,  255,  255 };
	static int wbr2g54[]	= { 255,   -1,   255,  255,  255,   -6,  255,  255,  255,  255,  255 };
	static int wzrg54[]	= {   2,    7,   255,  255,  255,    6,  255,  255,  255,  255,  255 };
	static int wr850g1[]	= {   7,    3,   255,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int wr850g2[]	= {   0,    1,   255,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int wtr54gs[]	= {   1,   -1,   255,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int dir320[]	= { -99,    1,     4,    3,  255,  255,  255,   -5,  255,  255,  255 };
	static int h618b[]	= { 255,   -1,   255,  255,  255,   -5,   -3,   -4,  255,  255,  255 };
	static int wl1600gl[]	= {   1,   -5,     0,  255,  255,    2,  255,  255,  255,  255,  255 };
	static int wrt310nv1[]	= { 255,    1,     9,    3,  255,  255,  255,  255,  255,  255,  255 };
	static int wrt160nv1[]	= { 255,    1,     5,    3,  255,  255,  255,  255,  255,  255,  255 };
#ifdef CONFIG_BCMWL5
	static int wnr3500[]	= { 255,  255,     2,  255,  255,   -1,  255,  255,  255,  255,  255 };
	static int wnr2000v2[]	= { 255,  255,   255,  255,  255,   -7,  255,  255,  255,  255,  255 };
	static int f7d[]	= { 255,  255,   255,  255,   12,   13,  255,   14,  255,  255,  255 };
	static int wrt160nv3[]	= { 255,    1,     4,    2,  255,  255,  255,  255,  255,  255,  255 };
	static int e900[]	= { 255,   -6,     8,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int e1000v2[]	= { 255,   -6,     8,    7,  255,  255,  255,  255,  255,  255,  255 };
	static int e3200[]	= { 255,   -3,   255,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int wrt320n[]	= { 255,    2,     3,    4,  255,  255,  255,  255,  255,  255,  255 };
	static int wrt610nv2[]	= { 255,    5,     3,    0,  255,  255,  255,   -7,  255,  255,  255 };
	static int e4200[]	= { 255,    5,    -3,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int rtn10u[]	= { 255,  255,   255,  255,  255,   -7,  255,   -8,  255,  255,  255 };
	static int rtn10p[]	= { 255,   -6,   255,  255,  255,   -7,  255,  255,  255,  255,  255 };
	static int rtn12b1[]	= {  -5,  255,   255,  255,  255,  255,  255,  225,  255,  255,  255 };
	static int rtn15u[]	= {   1,  255,     3,  255,  255,  255,  255,   -9,  255,  255,  255 };
	static int rtn53[]	= {   0,  -17,   255,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int l600n[]	= { 255,  255,   255,  255,  255,   -7,  255,   -8,  255,  255,  255 };
	static int dir620c1[]	= {  -6,   -8,   255,  255,  255,   -7,  255,  255,  255,  255,  255 };
	static int rtn66u[]	= { 255,  -12,   255,  255,  255,  255,  255,   15,  255,   13,  255 };
	static int w1800r[]	= { 255,  -13,   255,  255,  255,  255,  255,  -12,  255,   -5,  255 };
	static int d1800h[]	= { -12,  -13,     8,  255,  255,  -10,  255,   15,  255,   11,  255 };
	static int tdn6[]	= { 255,   -6,     8,  255,  255,  255,  255,  255,  255,  255,  255 };
#endif
#ifdef CONFIG_BCMWL6A
	static int ac68u[]	= { 254,  255,     4,  255,  255,   -3,  255,    0,   14,  254,  255 };
	static int ac56u[]	= { 255,  255,     1,  255,  255,   -3,    2,   14,    0,    6,  255 };
	static int n18u[]	= { 254,  255,     6,  255,  255,  -99,    9,    3,   14,  255,  255 };
	static int r6250[]	= {  11,    3,    15,  255,  255,    1,  255,    8,  255,  255,  255 };
	static int r6300v2[]	= {  11,    3,    10,  255,  255,    1,  255,    8,  255,  255,  255 };
	static int r6400[]	= {   9,    2,     7,  255,  -10,  -11,  255,   12,   13,    8,  255 };
	static int r7000[]	= {  13,    3,     9,  255,  -14,  -15,  255,   18,   17,   12,  255 };
	static int ac15[]	= { 255,  -99,   255,  255,  255,   -6,  255,  -14,  255,   -2,  255 };
	static int dir868[]	= { 255,    0,     3,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int ea6400[]	= { 255,  255,    -8,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int ea6500v2[]	= { 255,  255,     6,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int ea6700[]	= { 255,  255,    -8,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int ea6900[]	= { 255,  255,    -8,  255,  255,  255,  255,  255,  255,  255,  255 };
	static int ws880[]	= {   0,  255,   -12,  255,  255,  255,    1,   14,  255,    6,  255 };
	static int r1d[]	= { 255,  255,   255,  255,  255,    1,   -8,  255,  255,  255,  255 };
	static int wzr1750[]	= { 255,  255,   255,  255,  255,   -5,  255,  255,  255,  255,  255 };
#endif
#ifdef CONFIG_BCM7
	static int ac3200[]	= { 255,  -15,     5,  255,   14,   -3,  255,  255,  255,  255,  255 };
	static int r8000[]	= {  13,    3,     8,  255,  -14,  -15,  255,   18,   17,   12,   16 };
#endif
//				   ----  ----  ----- -----   ---  ----  ------ ---- ----    --   ---
//				   WLAN  DIAG  WHITE AMBER   DMZ  AOSS  BRIDGE USB2 USB3    5G   52G

	char s[16];
	int n;
	int b = 255, c = 255;
	int ret = 255;
	int model;

	if ((which < 0) || (which >= LED_COUNT)) return ret;

	/* get router model */
	model = get_model();

	/* stealth mode ON ? */
	if (nvram_match("stealth_mode", "1")) {
		/* turn off WLAN LEDs for some Asus Router: RT-N18U, RT-AC68U */
		if ((model == MODEL_RTN18U) ||
		    (model == MODEL_RTAC68U)) {
			do_led_nongpio(model, which, LED_OFF);
		}

		if (nvram_match("stealth_iled", "1") && which == LED_WHITE) { /* do not disable WAN / INTERNET LED and set LET_WHITE */
			/* nothing to do right now */
		}
		else {
			return ret; /* stealth mode ON: no LED work to do, set return value to 255 / disabled */
		}
	}

	switch (nvram_match("led_override", "1") ? MODEL_UNKNOWN : model) {
	case MODEL_WRT54G:
		if (check_hw_type() == HW_BCM4702) {
			/* G v1.x */
			if ((which != LED_DIAG) && (which != LED_DMZ)) return ret;
			b = (which == LED_DMZ) ? 1 : 4;
			if (mode != LED_PROBE) {
				if (f_read_string("/proc/sys/diag", s, sizeof(s)) > 0) {
					n = atoi(s);
					sprintf(s, "%u", mode ? (n | b) : (n & ~b));
					f_write_string("/proc/sys/diag", s, 0, 0);
				}
			}
			return b;
		}
		switch (which) {
		case LED_AMBER:
		case LED_WHITE:
			if (!supports(SUP_WHAM_LED)) return ret;
			break;
		}
		b = wrt54g[which];
		break;
	case MODEL_WTR54GS:
		b = wtr54gs[which];
		break;
	case MODEL_WRTSL54GS:
		b = wrtsl[which];
		break;
	case MODEL_WHRG54S:
	case MODEL_WHRHPG54:
	case MODEL_WHRG125:
		b = whrg54[which];
		break;
	case MODEL_WZRG54:
	case MODEL_WZRHPG54:
	case MODEL_WZRRSG54:
	case MODEL_WZRRSG54HP:
	case MODEL_WVRG54NF:
	case MODEL_WHR2A54G54:
	case MODEL_WHR3AG54:
	case MODEL_WZRG108:
		b = wzrg54[which];
		break;
/*
	case MODEL_WHR2A54G54:
		if (which != LED_DIAG) return ret;
		b = 7;
		break;
*/
	case MODEL_WBRG54:
		if (which != LED_DIAG) return ret;
		b = 7;
		break;
	case MODEL_WBR2G54:
		b = wbr2g54[which];
		break;
	case MODEL_WR850GV1:
		b = wr850g1[which];
		break;
	case MODEL_WR850GV2:
	case MODEL_WR100:
		b = wr850g2[which];
		break;
	case MODEL_WL500GP:
		if (which != LED_DIAG) return ret;
		b = -1;	/* power light */
		break;
	case MODEL_WL500W:
		if (which != LED_DIAG) return ret;
		b = -5;	/* power light */
		break;
	case MODEL_DIR320:
		b = dir320[which];
		break;
	case MODEL_H618B:
		b = h618b[which];
		break;
	case MODEL_WL1600GL:
		b = wl1600gl[which];
		break;
	case MODEL_WL500GPv2:
	case MODEL_WL500GD:
	case MODEL_WL520GU:
	case MODEL_WL330GE:
		if (which != LED_DIAG) return ret;
		b = -99;	/* Invert power light as diag indicator */
		break;
#ifdef CONFIG_BCMWL5
	case MODEL_RTN12:
		if (which != LED_DIAG) return ret;
		b = -2;	/* power light */
		break;
	case MODEL_RTN10:
	case MODEL_RTN16:
		if (which != LED_DIAG) return ret;
		b = -1;	/* power light */
		break;
	case MODEL_RTN15U:
		b = rtn15u[which];
		break;
	case MODEL_RTN53:
	case MODEL_RTN53A1:
		b = rtn53[which];
		break;
	case MODEL_RTN66U:
		b = rtn66u[which];
		break;
	case MODEL_W1800R:
		b = w1800r[which];
		break;
	case MODEL_D1800H:
		if (which == LED_DIAG) {
			/* power led gpio: 0x02 - white, 0x13 - red */
			b = (mode) ? 13 : 2;
			c = (mode) ? 2 : 13;
		} else
			b = d1800h[which];
		break;
	case MODEL_WNR3500L:
	case MODEL_WNR3500LV2:
		if (which == LED_DIAG) {
			/* power led gpio: 0x03 - green, 0x07 - amber */
			b = (mode) ? 7 : 3;
			c = (mode) ? 3 : 7;
		} else
			b = wnr3500[which];
		break;
	case MODEL_WNR2000v2:
		if (which == LED_DIAG) {
			/* power led gpio: 0x01 - green, 0x02 - amber */
			b = (mode) ? 2 : 1;
			c = (mode) ? 1 : 2;
		} else
			b = wnr2000v2[which];
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		if (which == LED_DIAG) {
			/* power led gpio: 10 - green, 11 - red */
			b = (mode) ? 11 : -10;
			c = (mode) ? -10 : 11;
		} else
			b = f7d[which];
		break;
	case MODEL_E1000v2:
		b = e1000v2[which];
		break;
	case MODEL_E900:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
		b = e900[which];
		break;
	case MODEL_E3200:
		b = e3200[which];
		break;
	case MODEL_WRT160Nv3:
		b = wrt160nv3[which];
		break;
	case MODEL_WRT320N:
		b = wrt320n[which];
		break;
	case MODEL_WRT610Nv2:
		b = wrt610nv2[which];
		break;
	case MODEL_E4200:
		b = e4200[which];
		break;
	case MODEL_RTN10U:
		b = rtn10u[which];
		break;
	case MODEL_RTN10P:
		b = rtn10p[which];
		break;
	case MODEL_RTN12B1:
		b = rtn12b1[which];
		break;
	case MODEL_L600N:
		b = l600n[which];
		break;
	case MODEL_DIR620C1:
		b = dir620c1[which];
	case MODEL_TDN60:	/* bwq518 */
	case MODEL_TDN6:
		b = tdn6[which];
		break;
#endif
#ifdef CONFIG_BCMWL6A
	case MODEL_RTAC68U:
		b = ac68u[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		break;
	case MODEL_RTAC56U:
		b = ac56u[which];
		break;
	case MODEL_RTN18U:
		b = n18u[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		break;
	case MODEL_R6250:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = r6250[which];
		}
		break;
	case MODEL_R6300v2:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else {
			b = r6300v2[which];
		}
		break;
	case MODEL_R6400:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else {
			b = r6400[which];
		}
		break;
	case MODEL_R7000:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 9; /* color white gpio 9 (active LOW) */
			c = 8; /* color amber gpio 8 (active LOW) */
		}
		else {
			b = r7000[which];
		}
		break;
	case MODEL_AC15:
		b = ac15[which];
		break;
	case MODEL_DIR868L:
		if (which == LED_DIAG) {
			b = 0; /* color amber gpio 0 (active LOW) */
			c = 2; /* color green gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 3; /* color green gpio 3 (active LOW) */
			c = 1; /* color amber gpio 1 (active LOW) */
		}
		else {
			b = dir868[which];
		}
		break;
	case MODEL_WS880:
		b = ws880[which];
		break;
	case MODEL_R1D:
		if (which == LED_DIAG) {
			/* power led gpio: -2 - orange, -3 - blue */
			b = (mode) ? 3 : 2;
			c = (mode) ? 2 : 3;
		}
		else {
			b = r1d[which];
		}
		break;
	case MODEL_EA6400:
		b = ea6400[which];
		break;
	case MODEL_EA6700:
		if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
			b = ea6500v2[which];
		}
		else {
			b = ea6700[which];
		}
		break;
	case MODEL_EA6900:
		b = ea6900[which];
		break;
	case MODEL_WZR1750:
		b = wzr1750[which];
		break;
#endif
#ifdef CONFIG_BCM7
	case MODEL_RTAC3200:
		b = ac3200[which];
		break;
	case MODEL_R8000:
		if (which == LED_DIAG) {
			b = 3; /* color amber gpio 3 (active LOW) */
			c = 2; /* color white gpio 2 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 8; /* color white gpio 8 (active LOW) */
			c = -9; /* color amber gpio 9 (active HIGH) */
		}
		else {
			b = r8000[which];
		}
		break;

#endif
/*
	case MODEL_RT390W:
		break;
*/
	case MODEL_MN700:
		if (which != LED_DIAG) return ret;
		b = 6;
		break;
	case MODEL_WLA2G54L:
		if (which != LED_DIAG) return ret;
		b = 1;
		break;
	case MODEL_WRT300N:
		if (which != LED_DIAG) return ret;
		b = 1;
		break;
	case MODEL_WRT310Nv1:
		b = wrt310nv1[which];
		break;
	case MODEL_WRT160Nv1:
		b = wrt160nv1[which];
		break;
	default:
		sprintf(s, "led_%s", led_names[which]);
		if (nvget_gpio(s, &b, &n)) {
			if ((mode != LED_PROBE) && (n)) mode = !mode;
			ret = (n) ? b : ((b) ? -b : -99);
			goto SET;
		}
		return ret;
	}

	ret = b;
	if (b < TOMATO_GPIO_MIN) {
		if (b == -99)
			b = TOMATO_GPIO_MIN;	/* -0 substitute */
		else
			b = -b;
	}
	else if (mode != LED_PROBE) {
		mode = !mode;
	}

SET:
	if (b <= TOMATO_GPIO_MAX) {
		if (mode != LED_PROBE) {
			gpio_write(1 << b, mode);

			if (c < TOMATO_GPIO_MIN) {
				if (c == -99)
					c = TOMATO_GPIO_MIN;
				else
					c = -c;
			}
			else
				mode = !mode;

			if (c <= TOMATO_GPIO_MAX) gpio_write(1 << c, mode);
		}
	}

	return ret;
}

void disable_led_wanlan(void) {
	system("/usr/sbin/et robowr 0x0 0x18 0x0100");	/* turn off all LAN and WAN LEDs Part 1/2 */
	system("/usr/sbin/et robowr 0x0 0x1a 0x0100");	/* turn off all LAN and WAN LEDs Part 2/2 */
}

void enable_led_wanlan(void) {
	system("/usr/sbin/et robowr 0x0 0x18 0x01ff");	/* turn on all LAN and WAN LEDs Part 1/2 */
	system("/usr/sbin/et robowr 0x0 0x1a 0x01ff");	/* turn on all LAN and WAN LEDs Part 2/2 */
}

void led_setup(void) {

	int model;

	/* get router model */
	model = get_model();

	/* stealth mode on ? */
	if (nvram_match("stealth_mode", "1")) {

		/* the following router do have LEDs for WLAN, WAN and LAN - see at the ethernet connectors or at the front panel / case */
		/* turn off non GPIO LEDs and some special cases like power LED - - do_led(...) will take care of the other ones */
		switch(model) {
		case MODEL_DIR868L:
			system("gpio enable 0");	/* disable power led color amber */
			break;
		case MODEL_AC15:
			system("gpio disable 0");	/* disable power led */
			disable_led_wanlan();
			break;
		case MODEL_R6250:
		case MODEL_R6300v2:
			system("gpio enable 3");	/* disable power led color amber */
			break;
		case MODEL_R6400:
			system("gpio enable 2");	/* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_R7000:
			system("gpio enable 3");	/* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_RTN18U:
			system("gpio enable 0");	/* disable power led color blue */
			break;
		case MODEL_RTAC56U:
			system("gpio enable 3");	/* disable power led color blue */
			disable_led_wanlan();
			break;
		case MODEL_RTAC68U:
			system("gpio enable 3");	/* disable power led */
			disable_led_wanlan();
			break;
		case MODEL_EA6400:
		case MODEL_EA6900:
			system("gpio disable 8");	/* disable LOGO led */
			disable_led_wanlan();
			break;
		case MODEL_EA6700:
			if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
				system("gpio enable 6");	/* disable LOGO led for EA6500 */
			}
			else {
				system("gpio disable 8");	/* disable LOGO led for EA6700 */
			}
			disable_led_wanlan();
			break;
		case MODEL_R8000:
			system("gpio disable 9");	/* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_RTAC3200:
			system("gpio enable 3");	/* disable power led */
			system("gpio disable 15");	/* disable button led */
			disable_led_wanlan();
			break;
		default:
			/* nothing to do right now */
			break;
		}
	}
	else {
		/* LED setup/config/preparation for some router models */
		if ((model == MODEL_RTAC56U)) {
			system("gpio disable 4");	/* enable power supply for all LEDs, except for PowerLED */
		}
		else if ((model == MODEL_DIR868L)) {
			/* activate WAN port led */
			system("gpio disable 1");	/* DIR868L: enable LED_WHITE / WAN LED with color amber (1); switch to color green (3) with WAN up */
		}
		else if ((model == MODEL_R6400)) {
			/* activate WAN port led */
			system("gpio disable 6");	/* R6400: enable LED_WHITE / WAN LED with color amber (6) if ethernet cable is connected; switch to color white (7) with WAN up */
		}
		else if ((model == MODEL_R7000)) {
			/* activate WAN port led */
			system("/usr/sbin/et robowr 0x0 0x10 0x3000");	/* basic LED setup, RT-N18U & RT-AC56U have 0x0220 for example */
			system("/usr/sbin/et robowr 0x0 0x12 0x78");
			system("/usr/sbin/et robowr 0x0 0x14 0x01");	/* force port 0 (WAN) to use LED function 1 (blink); 0 == blink off and 1 == blink on; bit 0 = port 0 */
			system("gpio disable 8");	/* R7000: enable LED_WHITE / WAN LED with color amber (8) if ethernet cable is connected; switch to color white (9) with WAN up */
		}
		else if ((model == MODEL_R8000)) {
			/* activate WAN port led - not the same like R7000 */
			system("/usr/sbin/et robowr 0x0 0x10 0x3000");
			system("/usr/sbin/et robowr 0x0 0x12 0x78");
			system("/usr/sbin/et robowr 0x0 0x14 0x10");	/* R8000 Netgear source - rc/rc.c */
			system("gpio enable 9");	/* R8000: enable LED_WHITE / WAN LED with color amber (GPIO 9, active HIGH) if ethernet cable is connected; switch to color white (GPIO 8, active LOW) with WAN up */
		}

		/* the following router do have LEDs for WLAN, WAN and LAN - see at the ethernet connectors or at the front panel / case */
		/* turn on WAN and LAN LEDs */
		switch(model) {
		case MODEL_AC15:
		case MODEL_R6400:
		case MODEL_R7000:
		case MODEL_RTAC56U:
		case MODEL_RTAC68U:
		case MODEL_EA6400:
		case MODEL_EA6700:
		case MODEL_EA6900:
		case MODEL_R8000:
		case MODEL_RTAC3200:
			enable_led_wanlan();
			break;
		default:
			/* nothing to do right now */
			break;
		}

	}
}

/* control non GPIO LEDs for some Asus Router: RT-N18U, RT-AC68U */
void do_led_nongpio(int model, int which, int mode) {

	switch(model) {
	case MODEL_RTN18U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 7");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
	case MODEL_RTAC68U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		else if (which == LED_5G) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth2 ledbh 10 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth2 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
	default:
	/* nothing to do right now */
	break;
	}

}
