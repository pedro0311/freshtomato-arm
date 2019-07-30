/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <trxhdr.h>
#include "shared.h"

/*
				HW_*                  boardtype    boardnum  boardrev  boardflags  others
				--------------------- ------------ --------- --------- ----------- ---------------
EA6400		    		BCM4708               0x0646       01        0x1100    0x0110      0:devid=0x43A9
EA6500v2	    		BCM4708               0xF646       01        0x1100    0x0110      0:devid=0x4332
EA6700		    		BCM4708               0xF646       01        0x1100    0x0110      0:devid=0x4332
EA6900		    		BCM4708               0xD646       01        0x1100    0x0110
EA6200		    		BCM47081A0            0xE646       20130125  0x1100
EA6350v1	    		BCM47081A0            0xE646       20140309  0x1200    0x00000110  0:devid=0x43A9

WZR-1750DHP	    		BCM4708               0xF646       00        0x1100    0x0110      0:devid=0x4332

Tenda AC15			BCM4708               0x0646       30        0x1100 //model=AC15V1.0
Tenda AC18			BCM4708               0x0646       30        0x1100 //model=AC18V1.0

TrendNET			BCM4708               0x0646       1234      0x1100    0x80001200

RT-N18U				BCM47081A0            0x0646       00        0x1100    0x00000110
RT-AC56U			BCM4708               0x0646	   00	     0x1100    0x00000110
RT-AC68U			BCM4708               0x0646       <MAC>     0x1100    0x00001000
RT-AC68P			BCM4709               0x0665       <MAC>     0x1103    0x00001000

R7000				BCM4709               0x0665       32        0x1301    0x1000
R6250				BCM4708               0x0646       679       0x1110 // same as R6300v2 well we use the same MODEL definition
R6300v2				BCM4708               0x0646       679       0x1110 // CH/Charter version has the same signature
R6400				BCM4708               0x0646       32        0x1601

DIR-868L			BCM4708               0x0646       24        0x1110
DIR-868LC1			BCM4708               0x0646       24        0x1101 //same as rev a/b but different boardrev
WS880				BCM4708               0x0646       1234      0x1101
R1D				BCM4709               0x0665       32        0x1301 //same as R7000

BFL_ENETADM	0x0080
BFL_ENETVLAN	0x0100
*/

int check_hw_type(void)
{
	const char *s;

	s = nvram_safe_get("boardtype");

	switch (strtoul(s, NULL, 0)) {
#ifdef CONFIG_BCMWL6A
	case 0x0646:	/* EA6400 */
	case 0x0665:	/* R7000,R1D */
	case 0xf646:	/* EA6700,WZR-1750, R6400 */
	case 0xd646:	/* EA6900 */
	case 0xe646:	/* EA6200, EA6350v1 */
		return HW_BCM4708; /* and also for 4709 right now!  */
#endif /* CONFIG_BCMWL6A */
	}

	return HW_UNKNOWN;
}

int get_model(void)
{
	int hw;
	hw = check_hw_type();

#ifdef CONFIG_BCMWL6A
	if (hw == HW_BCM4708) {
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("model", "R1D"))) return MODEL_R1D;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-N18U"))) return MODEL_RTN18U;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC56U"))) return MODEL_RTAC56U;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U;
		/* REMOVE: Same as RT-AC68U, no nvram "model=RT-AC68R" according to CFE for RT-AC68R */
//		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC68R"))) return MODEL_RTAC68U;
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H245T00_NETGEAR"))) return MODEL_R6250;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T00_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T70_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32"))) return MODEL_R6400;
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("boardnum", "32"))) return MODEL_R7000;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "24"))) return MODEL_DIR868L;
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "24"))) return MODEL_DIR868L;  /* rev c --> almost the same like rev a/b but different boardrev */
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "1234"))) return MODEL_WS880;
		if ((nvram_match("boardtype","0xE646")) && (nvram_match("boardnum", "20140309"))) return MODEL_EA6350v1;
		if ((nvram_match("boardtype","0xE646")) && (nvram_match("boardnum", "20130125"))) return MODEL_EA6350v1; /* EA6200 --> same like EA6350v1, AC1200 class router */
		if ((nvram_match("boardtype","0x0646")) && (nvram_match("boardnum", "01"))) return MODEL_EA6400;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "01"))) return MODEL_EA6700;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "00"))) return MODEL_WZR1750;
		if ((nvram_match("boardtype","0xD646")) && (nvram_match("boardrev", "0x1100"))) return MODEL_EA6900;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "AC15V1.0"))) return MODEL_AC15; /* Tenda AC15 */
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "AC18V1.0"))) return MODEL_AC18; /* Tenda AC18 */
	}
#endif /* CONFIG_BCMWL6A */

	return MODEL_UNKNOWN;
}

int supports(unsigned long attr)
{
	return (strtoul(nvram_safe_get("t_features"), NULL, 0) & attr) != 0;
}
