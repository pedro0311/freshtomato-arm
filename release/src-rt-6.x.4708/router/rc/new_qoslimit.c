/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate
	rate limit & connection limit by conanxu
	2011 modified by Victek & Shibby for 2.6 kernel
	last changed: 20110210
*/


#include "rc.h"

#include <sys/stat.h>

/*int  chain
1 = MANGLE
2 = NAT
*/

static const char *qoslimitfn = "/etc/qoslimit";

#define IP_ADDRESS	0
#define MAC_ADDRESS	1
#define IP_RANGE	2


void address_checker (int *address_type, char *ipaddr_old, char *ipaddr)
{
	char *second_part, *last_dot;
	int length_to_minus, length_to_dot;
	
	second_part = strchr(ipaddr_old, '-');
	if (second_part != NULL) {
		/* ip range */
		*address_type = IP_RANGE;
		if (strchr(second_part + 1, '.') != NULL)
			/* long notation */
			strcpy (ipaddr, ipaddr_old);
		else {
			/* short notation */
			last_dot = strrchr(ipaddr_old, '.');
			length_to_minus = second_part - ipaddr_old;
			length_to_dot = last_dot - ipaddr_old;
			strncpy(ipaddr, ipaddr_old, length_to_minus + 1);
			strncpy(ipaddr + length_to_minus + 1, ipaddr, length_to_dot + 1);
			strcpy(ipaddr + length_to_minus + length_to_dot + 2, second_part + 1); 
		}
	}
	else {
		/* mac address or ip address */
		if (strlen(ipaddr_old) != 17)
			/* IP_ADDRESS */
			*address_type = IP_ADDRESS;
		else
			/* MAC ADDRESS */
			*address_type = MAC_ADDRESS;

		strcpy (ipaddr, ipaddr_old);
	}
}

void ipt_qoslimit(int chain)
{
	char *buf;
	char *g;
	char *p;
	char *ibw, *obw;		/* bandwidth */
	char seq[4];			/* mark number */
	int iSeq = 10;
	char *ipaddr_old;
	char ipaddr[30];		/* ip address */
	char *dlrate, *dlceil;		/* guaranteed rate & maximum rate for download */
	char *ulrate, *ulceil;		/* guaranteed rate & maximum rate for upload */
	char *priority;			/* priority */
	char *lanipaddr;		/* lan ip address */
	char *lanmask;			/* lan netmask */
	char *lanX_ipaddr;		/* (br1 - br3) */
	char *lanX_mask;		/* (br1 - br3) */
	char *tcplimit, *udplimit;	/* tcp connection limit & udp packets per second */
	int priority_num;
	char *qosl_tcp, *qosl_udp;
	int i, address_type;

	if (!nvram_get_int("new_qoslimit_enable"))
		return;

	/* read qosrules from nvram */
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("wan_qos_ibw");	/* Read from QOS setting */
	obw = nvram_safe_get("wan_qos_obw");	/* Read from QOS setting */

	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");

	qosl_tcp = nvram_safe_get("qosl_tcp");
	qosl_udp = nvram_safe_get("qosl_udp");

	/* MANGLE */
	if (chain == 1) {
		if (nvram_get_int("qosl_enable") == 1)
			ipt_write("-A POSTROUTING ! -s %s/%s -d %s/%s -j MARK --set-mark 100\n"
			          "-A PREROUTING  -s %s/%s ! -d %s/%s -j MARK --set-mark 100\n",
			          lanipaddr, lanmask, lanipaddr, lanmask,
			          lanipaddr, lanmask, lanipaddr, lanmask);

		/* br1 */
		if (nvram_get_int("limit_br1_enable") == 1) {
			lanX_ipaddr = nvram_safe_get("lan1_ipaddr");
			lanX_mask = nvram_safe_get("lan1_netmask");

			ipt_write("-A POSTROUTING -d %s/%s -j MARK --set-mark 401\n"
			          "-A PREROUTING -s %s/%s -j MARK --set-mark 501\n",
			          lanX_ipaddr, lanX_mask,
			          lanX_ipaddr, lanX_mask);
		}

		/* br2 */
		if (nvram_get_int("limit_br2_enable") == 1) {
			lanX_ipaddr = nvram_safe_get("lan2_ipaddr");
			lanX_mask = nvram_safe_get("lan2_netmask");

			ipt_write("-A POSTROUTING -d %s/%s -j MARK --set-mark 601\n"
			          "-A PREROUTING -s %s/%s -j MARK --set-mark 701\n",
			          lanX_ipaddr, lanX_mask,
			          lanX_ipaddr, lanX_mask);
		}

		/* br3 */
		if (nvram_get_int("limit_br3_enable") == 1) {
			lanX_ipaddr = nvram_safe_get("lan3_ipaddr");
			lanX_mask = nvram_safe_get("lan3_netmask");

			ipt_write("-A POSTROUTING -d %s/%s -j MARK --set-mark 801\n"
			          "-A PREROUTING -s %s/%s -j MARK --set-mark 901\n",
			          lanX_ipaddr, lanX_mask,
			          lanX_ipaddr, lanX_mask);
		}
	}

	/* NAT */
	if (chain == 2) {
		if (nvram_get_int("qosl_enable") == 1) {
#ifndef TCONFIG_BCMARM
			if (nvram_get_int("qosl_tcp") > 0) {
				ipt_write("-A PREROUTING -s %s/%s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", lanipaddr, lanmask, qosl_tcp);
			}
#endif
			if (nvram_get_int("qosl_udp") > 0)
				ipt_write("-A PREROUTING -s %s/%s -p udp -m limit --limit %s/sec -j ACCEPT\n" , lanipaddr, lanmask, qosl_udp);
		}
	}

#ifdef TCONFIG_BCMARM
	/* Filter */
	if (chain == 3) {
		if (nvram_get_int("qosl_enable") == 1) {
			if (nvram_get_int("qosl_tcp") > 0)
				ipt_write("-A FORWARD -s %s/%s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n" , lanipaddr, lanmask, qosl_tcp);
		}
	}
#endif

	while (g) {
		/*
		ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit
		*/
		if ((p = strsep(&g, ">")) == NULL)
			break;
		i = vstrsep(p, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit);
		if (i != 8)
			continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5))
			continue;
		if (!strcmp(ipaddr_old, ""))
			continue;
		
		address_checker (&address_type, ipaddr_old, ipaddr);
		memset(seq, 0, 4);
		sprintf(seq, "%d", iSeq);
		iSeq++; 

		if (!strcmp(dlceil, ""))
			strcpy(dlceil, dlrate);

		if (strcmp(dlrate, "") && strcmp(dlceil, "")) {
			if (chain == 1) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A POSTROUTING ! -s %s/%s -d %s -j MARK --set-mark %s\n", lanipaddr, lanmask, ipaddr, seq);
						break;
					case MAC_ADDRESS:
						break;
					case IP_RANGE:
						ipt_write("-A POSTROUTING ! -s %s/%s -m iprange --dst-range  %s -j MARK --set-mark %s\n", lanipaddr, lanmask, ipaddr, seq);
						break;
				}
			}
		}

		if (!strcmp(ulceil, ""))
			strcpy(ulceil, ulrate);

		if (strcmp(ulrate, "") && strcmp(ulceil, "")) {
			if (chain == 1) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A PREROUTING -s %s ! -d %s/%s -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
					case MAC_ADDRESS:
						ipt_write("-A PREROUTING -m mac --mac-source %s ! -d %s/%s  -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
					case IP_RANGE:
						ipt_write("-A PREROUTING -m iprange --src-range %s ! -d %s/%s -j MARK --set-mark %s\n", ipaddr, lanipaddr, lanmask, seq);
						break;
				}
			}
		}

		if (atoi(tcplimit) > 0) {
#ifdef TCONFIG_BCMARM
			if (chain == 3) {
#else
			if (chain == 2) {
#endif
				switch (address_type) {
					case IP_ADDRESS:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -s %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -s %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
					case MAC_ADDRESS:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -m mac --mac-source %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -m mac --mac-source %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
					case IP_RANGE:
#ifdef TCONFIG_BCMARM
						ipt_write("-A FORWARD -m iprange --src-range %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#else
						ipt_write("-A PREROUTING -m iprange --src-range %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n", ipaddr, tcplimit);
#endif
						break;
				}
			}
		}

		if (atoi(udplimit) > 0){
			if (chain == 2) {
				switch (address_type) {
					case IP_ADDRESS:
						ipt_write("-A PREROUTING -s %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
					case MAC_ADDRESS:
						ipt_write("-A PREROUTING -m mac --mac-source %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
					case IP_RANGE:
						ipt_write("-A PREROUTING -m iprange --src-range %s -p udp -m limit --limit %s/sec -j ACCEPT\n", ipaddr, udplimit);
						break;
				}
			}
		}
	}
	free(buf);
}

/* read nvram into files */
void new_qoslimit_start(void)
{
	FILE *tc;
	char *buf;
	char *g;
	char *p;
	char *ibw, *obw;		/* bandwidth */
	char seq[4];			/* mark number */
	int iSeq = 10;
	char *ipaddr_old; 
	char ipaddr[30];		/* ip address */
	char *dlrate, *dlceil;		/* guaranteed rate & maximum rate for download */
	char *ulrate, *ulceil;		/* guaranteed rate & maximum rate for upload */
	char *priority;			/* priority */
	char *lanipaddr;		/* lan ip address */
	char *lanmask;			/* lan netmask */
	char *tcplimit, *udplimit;	/* tcp connection limit & udp packets per second */
	int priority_num;
	char *dlr, *dlc, *ulr, *ulc, *prio; /* download / upload - rate / ceiling / prio */
	int i, address_type;
	int s[6];
	char *waniface;

	if (!nvram_get_int("new_qoslimit_enable"))
		return;

	/* read qosrules from nvram */
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("wan_qos_ibw");
	obw = nvram_safe_get("wan_qos_obw");

	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");
	waniface = nvram_safe_get("wan_iface");

	dlr = nvram_safe_get("qosl_dlr");		/* download rate */
	dlc = nvram_safe_get("qosl_dlc");		/* download ceiling */
	ulr = nvram_safe_get("qosl_ulr");		/* upload rate */
	ulc = nvram_safe_get("qosl_ulc");		/* upload ceiling */
	prio = nvram_safe_get("limit_br0_prio");	/* priority */

	if ((tc = fopen(qoslimitfn, "w")) == NULL) {
		perror(qoslimitfn);
		return;
	}

	fprintf(tc, "#!/bin/sh\n"
	            "\n"
	            "TCA=\"tc class add dev br0\"\n"
	            "TFA=\"tc filter add dev br0\"\n"
	            "TQA=\"tc qdisc add dev br0\"\n"
	            "\n"
	            "SFQ=\"sfq perturb 10\"\n"
	            "\n"
	            "TCAU=\"tc class add dev %s\"\n"
	            "TFAU=\"tc filter add dev %s\"\n"
	            "TQAU=\"tc qdisc add dev %s\"\n"
	            "\n"
	            "tc qdisc del dev br0 root 2>/dev/null\n"
	            "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	            "\ttc qdisc del dev %s root 2>/dev/null\n"
	            "}\n"
	            "\n"
	            "tc qdisc add dev br0 root handle 1: htb\n"
	            "tc class add dev br0 parent 1: classid 1:1 htb rate %skbit\n"
	            "\n"
	            "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	            "\ttc qdisc add dev %s root handle 2: htb\n"
	            "\ttc class add dev %s parent 2: classid 2:1 htb rate %skbit\n"
	            "}\n"
	            "\n",
	            waniface,
	            waniface,
	            waniface,
	            waniface,
	            ibw,
	            waniface,
	            waniface, obw);

	if ((nvram_get_int("qosl_enable") == 1) && strcmp(dlr, "") && strcmp(ulr, "")) {
		if (!strcmp(dlc, ""))
			strcpy(dlc, dlr);
		if (!strcmp(ulc, ""))
			strcpy(ulc, ulr);

		fprintf(tc, "$TCA parent 1:1 classid 1:100 htb rate %skbit ceil %skbit prio %s\n"
		            "$TQA parent 1:100 handle 100: $SFQ\n"
		            "$TFA parent 1:0 prio 3 protocol all handle 100 fw flowid 1:100\n"
		            "\n"
		            "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
		            "\t$TCAU parent 2:1 classid 2:100 htb rate %skbit ceil %skbit prio %s\n"
		            "\t$TQAU parent 2:100 handle 100: $SFQ\n"
		            "\t$TFAU parent 2:0 prio 3 protocol all handle 100 fw flowid 2:100\n"
		            "}\n"
		            "\n",
		            dlr, dlc, prio,
		            ulr, ulc, prio);
	}

	while (g) {
		/*
		ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit
		*/
		if ((p = strsep(&g, ">")) == NULL)
			break;
		i = vstrsep(p, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit);
		if (i != 8)
			continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5))
			continue;
		if (!strcmp(ipaddr_old, ""))
			continue;

		address_checker(&address_type, ipaddr_old, ipaddr);
		memset(seq, 0, 4);
		sprintf(seq, "%d", iSeq);
		iSeq++;
		if (!strcmp(dlceil, ""))
			strcpy(dlceil, dlrate);

		if (strcmp(dlrate, "") && strcmp(dlceil, "")) {
			fprintf(tc, "$TCA parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
			            "$TQA parent 1:%s handle %s: $SFQ\n",
			            seq, dlrate, dlceil, priority,
			            seq, seq);

			if (address_type != MAC_ADDRESS)
				fprintf(tc, "$TFA parent 1:0 prio %s protocol all handle %s fw flowid 1:%s\n\n", priority, seq, seq);
			else if (address_type == MAC_ADDRESS) {
				sscanf(ipaddr, "%02X:%02X:%02X:%02X:%02X:%02X", &s[0], &s[1], &s[2], &s[3], &s[4], &s[5]);

				fprintf(tc, "$TFA parent 1:0 protocol all prio %s u32 match u16 0x0800 0xFFFF at -2 match u32 0x%02X%02X%02X%02X 0xFFFFFFFF at -12 match u16 0x%02X%02X 0xFFFF at -14 flowid 1:%s\n\n",
				            priority, s[2], s[3], s[4], s[5], s[0], s[1], seq);
			}
		}

		if (!strcmp(ulceil, ""))
			strcpy(ulceil, dlrate);

		if (strcmp(ulrate, "") && strcmp(ulceil, ""))
			fprintf(tc, "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
			            "\t$TCAU parent 2:1 classid 2:%s htb rate %skbit ceil %skbit prio %s\n"
			            "\t$TQAU parent 2:%s handle %s: $SFQ\n"
			            "\t$TFAU parent 2:0 prio %s protocol all handle %s fw flowid 2:%s\n"
			            "}\n"
			            "\n",
			            seq, ulrate, ulceil, priority,
			            seq, seq,
			            priority, seq, seq);
	}
	free(buf);

	/* limit br1 */
	if (nvram_get_int("limit_br1_enable") == 1) {
		dlr = nvram_safe_get("limit_br1_dlr");		/* download rate */
		dlc = nvram_safe_get("limit_br1_dlc");		/* download ceiling */
		ulr = nvram_safe_get("limit_br1_ulr");		/* upload rate */
		ulc = nvram_safe_get("limit_br1_ulc");		/* upload ceiling */
		prio = nvram_safe_get("limit_br1_prio");	/* priority */

		if (!strcmp(dlc, ""))
			strcpy(dlc, dlr);
		if (!strcmp(ulc, ""))
			strcpy(ulc, ulr);

		/* download for br1 */
		fprintf(tc, "TCA1=\"tc class add dev br1\"\n"
		            "TFA1=\"tc filter add dev br1\"\n"
		            "TQA1=\"tc qdisc add dev br1\"\n"
		            "tc qdisc del dev br1 root\n"
		            "tc qdisc add dev br1 root handle 4: htb\n"
		            "tc class add dev br1 parent 4: classid 4:1 htb rate %skbit\n"
		            "$TCA1 parent 4:1 classid 4:401 htb rate %skbit ceil %skbit prio %s\n"
		            "$TQA1 parent 4:401 handle 401: $SFQ\n"
		            "$TFA1 parent 4:0 prio %s protocol all handle 401 fw flowid 4:401\n",
		            ibw,
		            dlr, dlc, prio,
		            prio);

		/* upload for br1 */
		fprintf(tc, "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
		            "\t$TCAU parent 2:1 classid 2:501 htb rate %skbit ceil %skbit prio %s\n"
		            "\t$TQAU parent 2:501 handle 501: $SFQ\n"
		            "\t$TFAU parent 2:0 prio %s protocol all handle 501 fw flowid 2:501\n"
		            "}\n",
		            ulr, ulc, prio,
		            prio);
	}

	/* limit br2 */
	if (nvram_get_int("limit_br2_enable") == 1) {
		dlr = nvram_safe_get("limit_br2_dlr");		/* download rate */
		dlc = nvram_safe_get("limit_br2_dlc");		/* download ceiling */
		ulr = nvram_safe_get("limit_br2_ulr");		/* upload rate */
		ulc = nvram_safe_get("limit_br2_ulc");		/* upload ceiling */
		prio = nvram_safe_get("limit_br2_prio");	/* priority */

		if (!strcmp(dlc, ""))
			strcpy(dlc, dlr);
		if (!strcmp(ulc, ""))
			strcpy(ulc, ulr);

		/* download for br2 */
		fprintf(tc, "TCA2=\"tc class add dev br2\"\n"
		            "TFA2=\"tc filter add dev br2\"\n"
		            "TQA2=\"tc qdisc add dev br2\"\n"
		            "tc qdisc del dev br2 root\n"
		            "tc qdisc add dev br2 root handle 6: htb\n"
		            "tc class add dev br2 parent 6: classid 6:1 htb rate %skbit\n"
		            "$TCA2 parent 6:1 classid 6:601 htb rate %skbit ceil %skbit prio %s\n"
		            "$TQA2 parent 6:601 handle 601: $SFQ\n"
		            "$TFA2 parent 6:0 prio %s protocol all handle 601 fw flowid 6:601\n",
		            ibw,
		            dlr, dlc, prio,
		            prio);

		/* upload for br2 */
		fprintf(tc, "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
		            "\t$TCAU parent 2:1 classid 2:701 htb rate %skbit ceil %skbit prio %s\n"
		            "\t$TQAU parent 2:701 handle 701: $SFQ\n"
		            "\t$TFAU parent 2:0 prio %s protocol all handle 701 fw flowid 2:701\n"
		            "}\n",
		            ulr, ulc, prio,
		            prio);
	}

	/* limit br3 */
	if (nvram_get_int("limit_br3_enable") == 1) {
		dlr = nvram_safe_get("limit_br3_dlr");		/* download rate */
		dlc = nvram_safe_get("limit_br3_dlc");		/* download ceiling */
		ulr = nvram_safe_get("limit_br3_ulr");		/* upload rate */
		ulc = nvram_safe_get("limit_br3_ulc");		/* upload ceiling */
		prio = nvram_safe_get("limit_br3_prio");	/* priority */

		if (!strcmp(dlc, ""))
			strcpy(dlc, dlr);
		if (!strcmp(ulc, ""))
			strcpy(ulc, ulr);

		/* download for br3 */
		fprintf(tc, "TCA3=\"tc class add dev br3\"\n"
		            "TFA3=\"tc filter add dev br3\"\n"
		            "TQA3=\"tc qdisc add dev br3\"\n"
		            "tc qdisc del dev br3 root\n"
		            "tc qdisc add dev br3 root handle 8: htb\n"
		            "tc class add dev br3 parent 8: classid 8:1 htb rate %skbit\n"
		            "$TCA3 parent 8:1 classid 8:801 htb rate %skbit ceil %skbit prio %s\n"
		            "$TQA3 parent 8:801 handle 801: $SFQ\n"
		            "$TFA3 parent 8:0 prio %s protocol all handle 801 fw flowid 8:801\n",
		            ibw,
		            dlr, dlc, prio,
		            prio);

		/* upload for br3 */
		fprintf(tc, "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
		            "\t$TCAU parent 2:1 classid 2:901 htb rate %skbit ceil %skbit prio %s\n"
		            "\t$TQAU parent 2:901 handle 901: $SFQ\n"
		            "\t$TFAU parent 2:0 prio %s protocol all handle 901 fw flowid 2:901\n"
		            "}\n",
		            ulr, ulc, prio,
		            prio);
	}

	fclose(tc);

	chmod(qoslimitfn, 0700);

	/* fake start */
	eval((char *)qoslimitfn, "start");
}

void new_qoslimit_stop(void)
{
	FILE *f;
	char *s = "/tmp/qoslimittc_stop.sh";

	if ((f = fopen(s, "w")) == NULL) {
		perror(s);
		return;
	}

	fprintf(f, "#!/bin/sh\n"
	           "[ \"$(nvram get qos_enable)\" == \"0\" ] && {\n"
	           "\ttc qdisc del dev %s root\n"
	           "}\n"
	           "tc qdisc del dev br0 root\n"
	           "\n",
	           nvram_safe_get("wan_iface"));

	fclose(f);

	chmod(s, 0700);

	/* fake stop */
	eval(s, "stop");
}
/*
PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
           QD         |                         ^
                      |                         |
                      v                         |
                    INPUT (f)                 OUTPUT (mnf)
*/
