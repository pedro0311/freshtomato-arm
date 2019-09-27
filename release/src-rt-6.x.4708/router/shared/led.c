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


const char *led_names[] = {"wlan", "diag", "white", "amber", "dmz", "aoss", "bridge", "usb", "usb3", "5g"};



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

uint32_t set_gpio(uint32_t gpio, uint32_t value)
{
	uint32_t bit = 0;

	if ( gpio <= TOMATO_GPIO_MAX && gpio >= TOMATO_GPIO_MIN ) {
		bit = 1 << gpio;
//		_dprintf("set_gpio: %d %d\n", bit, value);
		gpio_write(bit, value);
		return 0;
	}
	else return 1;
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
//				    WLAN  DIAG  WHITE AMBER  DMZ  AOSS  BRIDGE USB2 USB3    5G
//				    ----- ----- ----- -----  ---  ----  ------ ---- ----    --
#ifdef CONFIG_BCMWL6A
	static int ac68u[]	= { 254,  255,     4,  255,  255,    3,  254,    0,   14,  254};
	static int ac56u[]	= { 254,  255,     1,  255,  255,    3,    2,   14,    0,    6};
	static int n18u[]	= { 254,  255,     6,  255,  255,    0,    9,    3,   14,  255};
	static int r6250[]	= {  11,    3,    15,  255,  255,    1,  255,    8,  255,  255};
	static int r6300v2[]	= {  11,    3,    10,  255,  255,    1,  255,    8,  255,  255};
	static int r6400[]	= {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r6400v2[]    = {   9,    2,     7,  255,  -10,  -11,  254,   12,   13,    8};
	static int r7000[]	= {  13,    3,     9,  255,  -14,  -15,  254,   18,   17,   12};
	static int ac15[]	= { 254,  -99,   255,  255,  255,   -6,  254,  -14,  255,   -2};
	static int ac18[]	= { 254,  -99,   255,  255,  255,   -6,  254,  -14,  255,   -2};
	static int dir868[]	= { 255,    0,     3,  255,  255,  255,  255,  255,  255,  255};
	static int ea6350v1[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6400[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6500v2[]	= { 255,  255,     6,  255,  255,  255,  254,  255,  255,  255};
	static int ea6700[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ea6900[]	= { 255,  255,    -8,  255,  255,  255,  254,  255,  255,  255};
	static int ws880[]	= {   0,  255,   -12,  255,  255,  255,    1,   14,  255,    6};
	static int r1d[]	= { 255,  255,   255,  255,  255,    1,   -8,  255,  255,  255};
#if 0 /* tbd. 8-Bit Shift Registers at arm branch M_ars */
	static int wzr1750[]	= {  -6,   -1,    -5,  255,  255,   -4,  255,  -99,  255,   -7}; /* 8 bit shift register (SPI GPIO 0 to 7), active HIGH */
#endif  /* tbd. 8-Bit Shift Registers at arm branch M_ars */
#endif
//				   ----  ----  ----- -----   ---  ----  ------ ---- ----    --
//				   WLAN  DIAG  WHITE AMBER   DMZ  AOSS  BRIDGE USB2 USB3    5G



	char s[16];
	int n;
	int b = 255, c = 255;
	int ret = 255;
	static int model = 0; /* initialize with 0 / MODEL_UNKNOWN */

	if ((which < 0) || (which >= LED_COUNT)) return ret;

	if (model == 0) { /* router model unknown OR detect router model for the first time at function do_led(). */
		/* get router model */
		model = get_model();
	}

	/* stealth mode ON ? */
	if (nvram_match("stealth_mode", "1")) {
		/* turn off WLAN LEDs for some Asus/Tenda Router: AC15, AC18, RT-N18U, RT-AC56U, RT-AC68U */
		switch (model) {
#ifdef CONFIG_BCMWL6A
			case MODEL_AC15:
			case MODEL_AC18:
			case MODEL_RTN18U:
			case MODEL_RTAC56U:
			case MODEL_RTAC68U:
#endif /* CONFIG_BCMWL6A */
#if defined(CONFIG_BCMWL6A) || defined(CONFIG_BCM7)
				do_led_nongpio(model, which, LED_OFF);
				break;
#endif /* CONFIG_BCMWL6A OR CONFIG_BCM7 */
			default:
				/* nothing to do right now */
				break;
		}

		if (nvram_match("stealth_iled", "1") && which == LED_WHITE) { /* do not disable WAN / INTERNET LED and set LED_WHITE */
			/* nothing to do right now */
		}
		else {
			return ret; /* stealth mode ON: no LED work to do, set return value to 255 / disabled */
		}
	}

	switch (nvram_match("led_override", "1") ? MODEL_UNKNOWN : model) {
#ifdef CONFIG_BCMWL6A
	case MODEL_RTAC68U:
		b = ac68u[which];
		if ((which == LED_WLAN) ||
		    (which == LED_5G)) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_RTAC56U:
		b = ac56u[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
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
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6400[which];
		}
		else {
			b = r6400[which];
		}
		break;
	case MODEL_R6400v2:
		if (which == LED_DIAG) {
			b = 2; /* color amber gpio 2 (active LOW) */
			c = 1; /* color white gpio 1 (active LOW) */
		}
		else if (which == LED_WHITE) {
			b = 7; /* color white gpio 7 (active LOW) */
			c = 6; /* color amber gpio 6 (active LOW) */
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r6400v2[which];
		}
		else {
			b = r6400v2[which];
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
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
			b = r7000[which];
		}
		else {
			b = r7000[which];
		}
		break;
	case MODEL_AC15:
		b = ac15[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_AC18:
		b = ac18[which];
		if (which == LED_WLAN) { /* non GPIO LED */
			do_led_nongpio(model, which, mode);
		}
		else if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
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
	case MODEL_EA6350v1:
		b = ea6350v1[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_EA6400:
		b = ea6400[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_EA6700:
		if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
			b = ea6500v2[which];
			if (which == LED_BRIDGE) { /* non GPIO LED */
				do_led_bridge(mode);
			}
		}
		else {
			b = ea6700[which];
			if (which == LED_BRIDGE) { /* non GPIO LED */
				do_led_bridge(mode);
			}
		}
		break;
	case MODEL_EA6900:
		b = ea6900[which];
		if (which == LED_BRIDGE) { /* non GPIO LED */
			do_led_bridge(mode);
		}
		break;
	case MODEL_WZR1750:
		/* tbd.: no support yet for 8-Bit Shift Registers at arm branch */
		b = 255; /* disabled */
		c = 255;
#if 0 /* tbd. 8-Bit Shift Registers at arm branch M_ars */
		if (which == LED_DIAG) {
			b = -1; /* color red gpio 1 (active HIGH) */
			c = 2; /* color white gpio 2 (active HIGH, inverted) */
		}
		else if (which == LED_AOSS) {
			b = -3; /* color blue gpio 3 (active HIGH) */
			c = 4; /* color amber gpio 4 (active HIGH, inverted) */
		}
		else {
			b = wzr1750[which];
		}
#endif  /* tbd. 8-Bit Shift Registers at arm branch M_ars */
		break;
#endif /* CONFIG_BCMWL6A */
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

void do_led_bridge(int mode) {
	if (mode == LED_ON) {
		enable_led_wanlan();
	}
	else if (mode == LED_OFF) {
		disable_led_wanlan();
	}
	else if (mode == LED_PROBE) {
		return;
	}
}

void led_setup(void) {

	int model;

	/* get router model */
	model = get_model();

	/* stealth mode on ? */
	if (nvram_match("stealth_mode", "1")) {

		/* the following router do have LEDs for WLAN, WAN and LAN - see at the ethernet connectors or at the front panel / case */
		/* turn off non GPIO LEDs and some special cases like power LED - - do_led(...) will take care of the other ones */
		switch (model) {
#ifdef CONFIG_BCMWL6A
		case MODEL_DIR868L:
			set_gpio(0, T_HIGH);		/* disable power led color amber */
			break;
		case MODEL_AC15:
			set_gpio(0, T_LOW);		/* disable sys led */
			disable_led_wanlan();
			break;
		case MODEL_AC18:
			set_gpio(0, T_LOW);		/* disable sys led */
			disable_led_wanlan();
			break;
		case MODEL_R6250:
		case MODEL_R6300v2:
			set_gpio(3, T_HIGH);		/* disable power led color amber */
			break;
		case MODEL_R6400:
		case MODEL_R6400v2:
			set_gpio(2, T_HIGH);		/* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_R7000:
			set_gpio(3, T_HIGH);		/* disable power led color amber */
			disable_led_wanlan();
			break;
		case MODEL_RTN18U:
			set_gpio(0, T_HIGH);		/* disable power led color blue */
			break;
		case MODEL_RTAC56U:
			set_gpio(3, T_HIGH);		/* disable power led color blue */
			disable_led_wanlan();
			break;
		case MODEL_RTAC68U:
			set_gpio(3, T_HIGH);		/* disable power led */
			disable_led_wanlan();
			break;
		case MODEL_EA6400:
		case MODEL_EA6900:
			set_gpio(8, T_LOW);		/* disable LOGO led */
			disable_led_wanlan();
			break;
		case MODEL_EA6700:
			if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL) { /* check for ea6500v2 --> same boardtype/num/rev like EA6700! */
				set_gpio(6, T_HIGH);		/* disable LOGO led for EA6500 */
			}
			else {
				set_gpio(8, T_LOW);		/* disable LOGO led for EA6700 */
			}
			disable_led_wanlan();
			break;
		case MODEL_WZR1750:
#if 0 /* tbd. 8-Bit Shift Registers at arm branch M_ars */
			set_gpio(1, T_LOW);		/* disable power led color red */
#endif /* tbd. 8-Bit Shift Registers at arm branch M_ars */
			break;
#endif /* CONFIG_BCMWL6A */
		default:
			/* nothing to do right now */
			break;
		}
	}
	else {
		/* LED setup/config/preparation for some router models */
		switch (model) {
#ifdef CONFIG_BCMWL6A
		case MODEL_DIR868L:
			/* activate WAN port led */
			set_gpio(1, T_LOW);		/* DIR868L: enable LED_WHITE / WAN LED with color amber (1); switch to color green (3) with WAN up */
			break;
		case MODEL_RTAC56U:
			set_gpio(4, T_LOW);		/* enable power supply for all LEDs, except for PowerLED */
			break;
		case MODEL_R6400:
		case MODEL_R6400v2:
			/* activate WAN port led */
			set_gpio(6, T_LOW);		/* R6400: enable LED_WHITE / WAN LED with color amber (6) if ethernet cable is connected; switch to color white (7) with WAN up */
			break;
		case MODEL_R7000:
			/* activate WAN port led */
			system("/usr/sbin/et robowr 0x0 0x10 0x3000");	/* basic LED setup, RT-N18U & RT-AC56U have 0x0220 for example */
			system("/usr/sbin/et robowr 0x0 0x12 0x78");
			system("/usr/sbin/et robowr 0x0 0x14 0x01");	/* force port 0 (WAN) to use LED function 1 (blink); 0 == blink off and 1 == blink on; bit 0 = port 0 */
			set_gpio(8, T_LOW);		/* R7000: enable LED_WHITE / WAN LED with color amber (8) if ethernet cable is connected; switch to color white (9) with WAN up */
			break;
#endif /* CONFIG_BCMWL6A */
		default:
			/* nothing to do right now */
			break;
		}
	}
}

/* control non GPIO LEDs for some Asus/Tenda Router: AC15, AC18, RT-N18U, RT-AC56U, RT-AC68U */
void do_led_nongpio(int model, int which, int mode) {

	switch (model) {
#ifdef CONFIG_BCMWL6A
	case MODEL_AC15:
	case MODEL_AC18:
	case MODEL_RTN18U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 10 7");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 10 0");
			else if (mode == LED_PROBE) return;
		}
		break;
	case MODEL_RTAC56U:
		if (which == LED_WLAN) {
			if (mode == LED_ON) system("/usr/sbin/wl -i eth1 ledbh 3 1");
			else if (mode == LED_OFF) system("/usr/sbin/wl -i eth1 ledbh 3 0");
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
#endif /* CONFIG_BCMWL6A */
	default:
		/* nothing to do right now */
		break;
	}

}
