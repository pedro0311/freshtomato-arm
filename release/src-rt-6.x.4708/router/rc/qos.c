/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 */


#include "rc.h"

#include <sys/stat.h>

#define CLASSES_NUM	10
static char qosfn[] = "/etc/wanX_qos";
static char qosdev[] = "iXXX";


void prep_qosstr(char *prefix)
{
	if (!strcmp(prefix, "wan")) {
		strcpy(qosfn, "/etc/wan_qos");
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb0");
#else
		strcpy(qosdev, "imq0");
#endif
	}
	else if (!strcmp(prefix, "wan2")) {
		strcpy(qosfn, "/etc/wan2_qos");
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb1");
#else
		strcpy(qosdev, "imq1");
#endif
	}
#ifdef TCONFIG_MULTIWAN
	else if (!strcmp(prefix, "wan3")) {
		strcpy(qosfn, "/etc/wan3_qos");
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb2");
#else
		strcpy(qosdev, "imq2");
#endif
	}
	else if (!strcmp(prefix, "wan4")) {
		strcpy(qosfn, "/etc/wan4_qos");
#ifdef TCONFIG_BCMARM
		strcpy(qosdev, "ifb3");
#else
		strcpy(qosdev, "imq3");
#endif
	}
#endif /* TCONFIG_MULTIWAN */
}

/* in mangle table */
void ipt_qos(void)
{
	char *buf;
	char *g;
	char *p;
	char *addr_type, *addr;
	char *proto;
	char *port_type, *port;
	char *class_prio;
	char *ipp2p, *layer7;
	char *bcount;
	char *dscp;
	char *desc;
	int class_num;
	int proto_num;
	int v4v6_ok;
	int i;
	char sport[192];
	char saddr[256];
	char end[256];
	char s[32];
	char app[128];
	int inuse;
	const char *chain;
	unsigned long min;
	unsigned long max;
	unsigned long prev_max;
	int gum;
	const char *qface;
	int sizegroup;
	int class_flag;
	int rule_num;
#ifndef TCONFIG_BCMARM
	int qosDevNumStr = 0;
#endif

	if (!nvram_get_int("qos_enable"))
		return;

	inuse = 0;
	gum = 0x100;
	sizegroup = 0;
	prev_max = 0;
	rule_num = 0;

	ip46t_write(":QOSO - [0:0]\n"
	            "-A QOSO -j CONNMARK --restore-mark --mask 0xff\n"
	            "-A QOSO -m connmark ! --mark 0/0x0f00 -j RETURN\n");

	g = buf = strdup(nvram_safe_get("qos_orules"));
	while (g) {

		/*
		 * addr_type<addr<proto<port_type<port<ipp2p<L7<bcount<dscp<class_prio<desc
		 *
		 * addr_type:
		 * 	0 = any
		 * 	1 = dest ip
		 * 	2 = src ip
		 * 	3 = src mac
		 * addr:
		 * 	ip/mac if addr_type == 1-3
		 * proto:
		 * 	0-65535 = protocol
		 * 	-1 = tcp or udp
		 * 	-2 = any protocol
		 * port_type:
		 * 	if proto == -1,tcp,udp:
		 * 		d = dest
		 * 		s = src
		 * 		x = both
		 * 		a = any
		 * port:
		 * 	port # if proto == -1,tcp,udp
		 * bcount:
		 * 	min:max
		 * 	blank = none
		 * dscp:
		 * 	empty - any
		 * 	numeric (0:63) - dscp value
		 * 	afXX, csX, be, ef - dscp class
		 * class_prio:
		 * 	0-10
		 * 	-1 = disabled
		 *
		 */

		if ((p = strsep(&g, ">")) == NULL)
			break;

		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type, &port, &ipp2p, &layer7, &bcount, &dscp, &class_prio, &desc);
		rule_num++;
		if (i == 10) {
			/* fixup < v1.28.XX55 */
			desc = class_prio;
			class_prio = dscp;
			dscp = "";
		}
		else if (i == 9) {
			/* fixup < v0.08 - temp */
			desc = class_prio;
			class_prio = bcount;
			bcount = "";
			dscp = "";
		}
		else if (i != 11)
			continue;

		class_num = atoi(class_prio);
		if ((class_num < 0) || (class_num > 9))
			continue;

		i = 1 << class_num;
		++class_num;

		if ((inuse & i) == 0)
			inuse |= i;

		v4v6_ok = IPT_V4;
#ifdef TCONFIG_IPV6
		if (ipv6_enabled())
			v4v6_ok |= IPT_V6;
#endif
		class_flag = gum;

		saddr[0] = '\0';
		end[0] = '\0';
		/* mac or ip address */
		if ((*addr_type == '1') || (*addr_type == '2')) { /* match ip */
			v4v6_ok &= ipt_addr(saddr, sizeof(saddr), addr, (*addr_type == '1') ? "dst" : "src", v4v6_ok, (v4v6_ok==IPT_V4), "QoS", desc);
			if (!v4v6_ok)
				continue;
		}
		else if (*addr_type == '3') { /* match mac */
			sprintf(saddr, "-m mac --mac-source %s", addr); /* (-m mac modified, returns !match in OUTPUT) */
		}

		/* IPP2P/Layer7 */
		memset(app, 0, 128);
		if (ipt_ipp2p(ipp2p, app))
			v4v6_ok &= ~IPT_V6;
		else
			ipt_layer7(layer7, app);

		if (app[0]) {
			v4v6_ok &= ~IPT_V6; /* temp: l7 not working either! */
			class_flag = 0x100;
			/* IPP2P and L7 rules may need more than one packet before matching
			 * so port-based rules that come after them in the list can't be sticky
			 *  or else these rules might never match
			 */
			gum = 0;
#ifdef TCONFIG_BCMARM
			strcat(saddr, app);
#else
			strcpy(end, app);
#endif
		}

		/* dscp */
		memset(s, 0, 32);
		if (ipt_dscp(dscp, s))
#ifdef TCONFIG_BCMARM
			strcat(saddr, s);
#else
			strcat(end, s);
#endif

		/* -m connbytes --connbytes x:y --connbytes-dir both --connbytes-mode bytes */
		if (*bcount) {
			min = strtoul(bcount, &p, 10);
			if (*p != 0) {
				strcat(saddr, " -m connbytes --connbytes-mode bytes --connbytes-dir both --connbytes ");
				++p;
				if (*p == 0)
					sprintf(saddr + strlen(saddr), "%lu:", min * 1024);
				else {
					max = strtoul(p, NULL, 10);
					sprintf(saddr + strlen(saddr), "%lu:%lu", min * 1024, (max * 1024) - 1);
					if (gum) {
						if (!sizegroup) {
							/* create table of connbytes sizes, pass appropriate connections there and only continue processing them if mark was wiped */
							ip46t_write(":QOSSIZE - [0:0]\n"
							            "-I QOSO 3 -m connmark ! --mark 0/0xff000 -j QOSSIZE\n"
							            "-I QOSO 4 -m connmark ! --mark 0/0xff000 -j RETURN\n");
						}
						if (max != prev_max && sizegroup < 255) {
							class_flag = ++sizegroup << 12;
							prev_max = max;
							ip46t_flagged_write(v4v6_ok, "-A QOSSIZE -m connmark --mark 0x%x/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
							                             "--connbytes %lu: -j CONNMARK --set-return 0x00000/0xFF\n", (sizegroup << 12), (max * 1024));
#ifdef TCONFIG_BCMARM
							ip46t_flagged_write(v4v6_ok, "-A QOSSIZE -m connmark --mark 0x%x/0xff000 -m connbytes --connbytes-mode bytes --connbytes-dir both "
							                             "--connbytes %lu: -j RETURN\n", (sizegroup << 12), (max * 1024));
#endif
						}
						else
							class_flag = sizegroup << 12;
					}
				}
			}
			else
				bcount = "";
		}

		chain = "QOSO";
		class_num |= class_flag;
		class_num |= rule_num << 20;
		sprintf(end + strlen(end), " -j CONNMARK --set-return 0x%x/0xFF\n", class_num);

		/* protocol & ports */
		proto_num = atoi(proto);
		if (proto_num > -2) {
			if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
				if (*port_type != 'a') {
					if ((*port_type == 'x') || (strchr(port, ',')))
						/* dst-or-src port matches, and anything with multiple lists "," use multiport */
						sprintf(sport, "-m multiport --%sports %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
					else
						/* single or simple x:y range, use built-in tcp/udp match */
						sprintf(sport, "--%sport %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
				}
				else
					sport[0] = 0;

				if (proto_num != 6) {
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "udp", sport, saddr, end);
#ifdef TCONFIG_BCMARM
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "udp", sport, saddr);
#endif
				}
				if (proto_num != 17) {
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "tcp", sport, saddr, end);
#ifdef TCONFIG_BCMARM
					ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s -j RETURN\n", chain, "tcp", sport, saddr);
#endif
				}
			}
			else {
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s %s", chain, proto_num, saddr, end);
#ifdef TCONFIG_BCMARM
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s -j RETURN\n", chain, proto_num, saddr);
#endif
			}
		}
		else { /* any protocol */
			ip46t_flagged_write(v4v6_ok, "-A %s %s %s", chain, saddr, end);
#ifdef TCONFIG_BCMARM
			ip46t_flagged_write(v4v6_ok, "-A %s %s -j RETURN\n", chain, saddr);
#endif
		}
	}
	free(buf);

	i = nvram_get_int("qos_default");
	if ((i < 0) || (i > 9))
		i = 3; /* "low" */

	class_num = i + 1;
	class_num |= 0xFF00000; /* use rule_num=255 for default */
	ip46t_write("-A QOSO -j CONNMARK --set-return 0x%x\n", class_num);
#ifdef TCONFIG_BCMARM
	ip46t_write("-A QOSO -j RETURN\n");
#endif

	qface = wanfaces.iface[0].name;
	ipt_write("-A FORWARD -o %s -j QOSO\n"
	          "-A OUTPUT -o %s -j QOSO\n"
#ifdef TCONFIG_BCMARM
	          "-A FORWARD -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n"
	          "-A OUTPUT -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n",
	          qface, qface
#endif
	          ,qface, qface);

	if (check_wanup("wan2")) {
		qface = wan2faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
#ifdef TCONFIG_BCMARM
		          "-A FORWARD -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n"
		          "-A OUTPUT -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n",
		          qface, qface
#endif
		          ,qface, qface);
	}
#ifdef TCONFIG_MULTIWAN
	if (check_wanup("wan3")) {
		qface = wan3faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
#ifdef TCONFIG_BCMARM
		          "-A FORWARD -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n"
		          "-A OUTPUT -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n",
		          qface, qface
#endif
		          ,qface, qface);
	}
	if (check_wanup("wan4")) {
		qface = wan4faces.iface[0].name;
		ipt_write("-A FORWARD -o %s -j QOSO\n"
		          "-A OUTPUT -o %s -j QOSO\n"
#ifdef TCONFIG_BCMARM
		          "-A FORWARD -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n"
		          "-A OUTPUT -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n",
		          qface, qface
#endif
		          ,qface, qface);
	}
#endif /* TCONFIG_MULTIWAN */

#ifdef TCONFIG_IPV6
	if (*wan6face)
		ip6t_write("-A FORWARD -o %s -j QOSO\n"
		           "-A OUTPUT -o %s -p icmpv6 -j RETURN\n"
		           "-A OUTPUT -o %s -j QOSO\n"
#ifdef TCONFIG_MULTIWAN
		           "-A FORWARD -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n"
		           "-A OUTPUT -o %s -m connmark ! --mark 0 -j CONNMARK --save-mark\n",
		           wan6face, wan6face
#endif
		           ,wan6face, wan6face, wan6face);
#endif /* TCONFIG_IPV6 */

	inuse |= (1 << i) | 1; /* default and highest are always built */
	memset(s, 0, 32);
	sprintf(s, "%d", inuse);
	nvram_set("qos_inuse", s);

	g = buf = strdup(nvram_safe_get("qos_irates"));
	for (i = 0; i < CLASSES_NUM; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
			continue;
		if ((inuse & (1 << i)) == 0)
			continue;
		
		unsigned int rate;
		unsigned int ceil;
		
		/* check if we've got a percentage definition in the form of "rate-ceiling" and that rate > 1 */
		if ((sscanf(p, "%u-%u", &rate, &ceil) == 2) && (rate >= 1)) {
			qface = wanfaces.iface[0].name;
			ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);
#ifdef TCONFIG_BCMARM
			ipt_write("-A PREROUTING -i %s -j RETURN\n", qface);
#endif

			if (check_wanup("wan2")) {
				qface = wan2faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);
#ifdef TCONFIG_BCMARM
				ipt_write("-A PREROUTING -i %s -j RETURN\n", qface);
#endif
			}

#ifdef TCONFIG_MULTIWAN
			if (check_wanup("wan3")) {
				qface = wan3faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);
#ifdef TCONFIG_BCMARM
				ipt_write("-A PREROUTING -i %s -j RETURN\n", qface);
#endif
			}
			if (check_wanup("wan4")) {
				qface = wan4faces.iface[0].name;
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);
#ifdef TCONFIG_BCMARM
				ipt_write("-A PREROUTING -i %s -j RETURN\n", qface);
#endif
			}
#endif

#ifndef TCONFIG_BCMARM
#ifdef TCONFIG_PPTPD
			if (nvram_get_int("pptp_client_enable") && !nvram_match("pptp_client_iface", "")) {
				qface = nvram_safe_get("pptp_client_iface");
				ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);
			}
#endif /* TCONFIG_PPTPD */

			if (nvram_get_int("qos_udp")) {
				qface = wanfaces.iface[0].name;
				qosDevNumStr = 0;
				ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */

				if (check_wanup("wan2")) {
					qface = wan2faces.iface[0].name;
					qosDevNumStr = 1;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
#ifdef TCONFIG_MULTIWAN
				if (check_wanup("wan3")) {
					qface = wan3faces.iface[0].name;
					qosDevNumStr = 2;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
				if (check_wanup("wan4")) {
					qface = wan4faces.iface[0].name;
					qosDevNumStr = 3;
					ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass only tcp */
				}
#endif /* TCONFIG_MULTIWAN */
			}
			else {
				qface = wanfaces.iface[0].name;
				qosDevNumStr = 0;
				ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */

				if (check_wanup("wan2")) {
					qface = wan2faces.iface[0].name;
					qosDevNumStr = 1;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
#ifdef TCONFIG_MULTIWAN
				if (check_wanup("wan3")) {
					qface = wan3faces.iface[0].name;
					qosDevNumStr = 2;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
				if (check_wanup("wan4")) {
					qface = wan4faces.iface[0].name;
					qosDevNumStr = 3;
					ipt_write("-A PREROUTING -i %s -j IMQ --todev %d\n", qface, qosDevNumStr); /* pass everything thru ingress */
				}
#endif /* TCONFIG_MULTIWAN */
			}
#endif /* !TCONFIG_BCMARM */

#ifdef TCONFIG_IPV6
			if (*wan6face) {
				ip6t_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", wan6face);
#ifdef TCONFIG_BCMARM
				ip6t_write("-A PREROUTING -i %s -j RETURN\n", wan6face);
#else
				qosDevNumStr = 0;
				if (nvram_get_int("qos_udp"))
						ip6t_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %d\n", wan6face, qosDevNumStr); /* pass only tcp */
				else
						ip6t_write("-A PREROUTING -i %s -j IMQ --todev %d\n", wan6face, qosDevNumStr); /* pass everything thru ingress */
#endif /* TCONFIG_BCMARM */
			}
#endif /* TCONFIG_IPV6 */
			break;
		}
	}
	free(buf);
}

static unsigned calc(unsigned bw, unsigned pct)
{
	unsigned n = ((unsigned long)bw * pct) / 100;

	return (n < 2) ? 2 : n;
}

void start_qos(char *prefix)
{
	int i;
	char *buf, *g, *p, *qos;
	unsigned int rate;
	unsigned int ceil;
	unsigned int bw;
	unsigned int incomingBWkbps;
	unsigned int mtu;
	unsigned int r2q;
	unsigned int qosDefaultClassId;
	unsigned int overhead;
	FILE *f;
	int x;
	int inuse;
	char s[256];
	int first;
	char burst_root[32];
	char burst_leaf[32];
	char tmp[100];

	/* Network Congestion Control */
	x = nvram_get_int("ne_vegas");
	if (x) {
		char alpha[10], beta[10], gamma[10];
		memset(alpha, 0, 10);
		sprintf(alpha, "alpha=%d", nvram_get_int("ne_valpha"));
		memset(beta, 0, 10);
		sprintf(beta, "beta=%d", nvram_get_int("ne_vbeta"));
		memset(gamma, 0, 10);
		sprintf(gamma, "gamma=%d", nvram_get_int("ne_vgamma"));
		modprobe("tcp_vegas", alpha, beta, gamma);
		f_write_procsysnet("ipv4/tcp_congestion_control", "vegas");
	}
	else {
		modprobe_r("tcp_vegas");
		f_write_procsysnet("ipv4/tcp_congestion_control",
#ifdef TCONFIG_BCMARM
		               "cubic"
#else
		               "reno"
#endif
		               );
	}

	if (!nvram_get_int("qos_enable"))
		return;

	qosDefaultClassId = (nvram_get_int("qos_default") + 1) * 10;
	incomingBWkbps = strtoul(nvram_safe_get(strcat_r(prefix, "_qos_ibw", tmp)), NULL, 10);

	prep_qosstr(prefix);

	if ((f = fopen(qosfn, "w")) == NULL) {
		perror(qosfn);
		return;
	}

	i = nvram_get_int("qos_burst0");
	if (i > 0)
		sprintf(burst_root, "burst %dk", i);
	else
		burst_root[0] = 0;

	i = nvram_get_int("qos_burst1");
	if (i > 0)
		sprintf(burst_leaf, "burst %dk", i);
	else
		burst_leaf[0] = 0;

	mtu = strtoul(nvram_safe_get("wan_mtu"), NULL, 10);
	bw = strtoul(nvram_safe_get(strcat_r(prefix, "_qos_obw", tmp)), NULL, 10);
	overhead = strtoul(nvram_safe_get("atm_overhead"), NULL, 10);
	r2q = 10;

	if ((bw * 1000) / (8 * r2q) < mtu) {
		r2q = (bw * 1000) / (8 * mtu);
		if (r2q < 1)
			r2q = 1;
	}
	else if ((bw * 1000) / (8 * r2q) > 60000)
		r2q = (bw * 1000) / (8 * 60000) + 1;

	x = nvram_get_int("qos_pfifo");
	if (x == 1)
		qos = "pfifo limit 256";
	else if (x == 2)
		qos = "codel";
	else if (x == 3)
		qos = "fq_codel";
	else
		qos = "sfq perturb 10";

	fprintf(f, "#!/bin/sh\n"
	           "WAN_DEV=%s\n"
	           "QOS_DEV=%s\n"
	           "TQA=\"tc qdisc add dev $WAN_DEV\"\n"
	           "TCA=\"tc class add dev $WAN_DEV\"\n"
	           "TFA=\"tc filter add dev $WAN_DEV\"\n"
	           "TQA_QOS=\"tc qdisc add dev $QOS_DEV\"\n"
	           "TCA_QOS=\"tc class add dev $QOS_DEV\"\n"
	           "TFA_QOS=\"tc filter add dev $QOS_DEV\"\n"
	           "Q=\"%s\"\n"
	           "\n"
	           "case \"$1\" in\n"
	           "start)\n"
	           "\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
	           "\t$TQA root handle 1: htb default %u r2q %u\n"
	           "\t$TCA parent 1: classid 1:1 htb rate %ukbit ceil %ukbit %s",
	           get_wanface(prefix),
	           qosdev,
	           qos,
	           qosDefaultClassId, r2q,
	           bw, bw, burst_root);

	if (overhead > 0)
#ifdef TCONFIG_BCMARM
		fprintf(f, " overhead %u linklayer atm", overhead);
#else
		fprintf(f, " overhead %u atm", overhead);
#endif

	fprintf(f, "\n");

	inuse = nvram_get_int("qos_inuse");

	g = buf = strdup(nvram_safe_get("qos_orates"));
	for (i = 0; i < CLASSES_NUM; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
			break;

		if ((inuse & (1 << i)) == 0)
			continue;

		/* check if we've got a percentage definition in the form of "rate-ceiling" */
		if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1))
			continue; /* 0=off */

		if (ceil > 0)
			sprintf(s, "ceil %ukbit", calc(bw, ceil));
		else
			s[0] = 0;

		x = (i + 1) * 10;

		fprintf(f, "# egress %d: %u-%u%%\n"
		           "\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u",
		           i, rate, ceil,
		           x, calc(bw, rate), s, burst_leaf, (i + 1), mtu);

		if (overhead > 0)
#ifdef TCONFIG_BCMARM
			fprintf(f, " overhead %u linklayer atm", overhead);
#else
			fprintf(f, " overhead %u atm", overhead);
#endif

		fprintf(f, "\n\t$TQA parent 1:%d handle %d: $Q\n"
		           "\t$TFA parent 1: prio %d protocol ip handle %d fw flowid 1:%d\n",
		           x, x,
		           x, (i + 1), x);

#ifdef TCONFIG_IPV6
		fprintf(f, "\t$TFA parent 1: prio %d protocol ipv6 handle %d fw flowid 1:%d\n",
		           x + 100, (i + 1), x);
#endif
	}
	free(buf);

	/*
	 * 10000 = ACK
	 * 00100 = RST
	 * 00010 = SYN
	 * 00001 = FIN
	*/

	if (nvram_get_int("qos_ack"))
		fprintf(f, "\n\t$TFA parent 1: prio 14 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x10 0xff at 33 "		/* ACK only */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_syn"))
		fprintf(f, "\n\t$TFA parent 1: prio 15 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x02 0x02 at 33 "		/* SYN,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_fin"))
		fprintf(f, "\n\t$TFA parent 1: prio 17 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x01 0x01 at 33 "		/* FIN,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_rst"))
		fprintf(f, "\n\t$TFA parent 1: prio 19 protocol ip u32 "
		           "match ip protocol 6 0xff "		/* TCP */
		           "match u8 0x05 0x0f at 0 "		/* IP header length */
		           "match u16 0x0000 0xffc0 at 2 "	/* total length (0-63) */
		           "match u8 0x04 0x04 at 33 "		/* RST,* */
		           "flowid 1:10\n");

	if (nvram_get_int("qos_icmp"))
		fprintf(f, "\n\t$TFA parent 1: prio 13 protocol ip u32 match ip protocol 1 0xff flowid 1:10\n");


	/*
	 * INCOMING TRAFFIC SHAPING
	 */
	
	first = 1;
	overhead = strtoul(nvram_safe_get("atm_overhead"), NULL, 10);

	g = buf = strdup(nvram_safe_get("qos_irates"));
	
	for (i = 0; i < CLASSES_NUM; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
			break;

		if ((inuse & (1 << i)) == 0)
			continue;

		/* check if we've got a percentage definition in the form of "rate-ceiling" */
		if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1))
			continue; /* 0=off */

		/* class ID */
		unsigned int classid = ((unsigned int)i + 1) * 10;

		/* priority */
		unsigned int priority = (unsigned int)i + 1; /* prios 1-10 */

		/* rate in kb/s */
		unsigned int rateInkbps = calc(incomingBWkbps, rate);

		/* ceiling in kb/s */
		unsigned int ceilingInkbps = calc(incomingBWkbps, ceil);

		r2q = 10;
		if ((incomingBWkbps * 1000) / (8 * r2q) < mtu) {
			r2q = (incomingBWkbps * 1000) / (8 * mtu);
			if (r2q < 1)
				r2q = 1;
		} 
		else if ((incomingBWkbps * 1000) / (8 * r2q) > 60000) 
			r2q = (incomingBWkbps * 1000) / (8 * 60000) + 1;

		if (first) {
			first = 0;
#ifdef TCONFIG_BCMARM
			fprintf(f, "\n\ttc qdisc del dev $WAN_DEV ingress 2>/dev/null\n"
			           "\t$TQA handle ffff: ingress\n");
#endif
			fprintf(f, "\n\tip link set $QOS_DEV up\n"
			           "\ttc qdisc del dev $QOS_DEV 2>/dev/null\n"
			           "\t$TQA_QOS handle 1: root htb default %u r2q %u\n"
			           "\t$TCA_QOS parent 1: classid 1:1 htb rate %ukbit ceil %ukbit",
			           qosDefaultClassId, r2q,
			           incomingBWkbps,
			           incomingBWkbps);

			if (overhead > 0)
#ifdef TCONFIG_BCMARM
				fprintf(f, " overhead %u linklayer atm", overhead);
#else
				fprintf(f, " overhead %u atm", overhead);
#endif

#ifdef TCONFIG_BCMARM
			fprintf(f, "\n\n\t$TFA parent ffff: protocol ip prio 10 u32 match ip %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#ifdef TCONFIG_IPV6
			fprintf(f, "\t$TFA parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action mirred egress redirect dev $QOS_DEV\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
#endif
#endif /* TCONFIG_BCMARM */

		}

		fprintf(f, "\n\t# class id %u: rate %ukbit ceil %ukbit\n"
		           "\t$TCA_QOS parent 1:1 classid 1:%u htb rate %ukbit ceil %ukbit prio %u quantum %u",
		           classid, rateInkbps, ceilingInkbps,
		           classid, rateInkbps, ceilingInkbps, priority, mtu);

		if (overhead > 0)
#ifdef TCONFIG_BCMARM
			fprintf(f, " overhead %u linklayer atm", overhead);
#else
			fprintf(f, " overhead %u atm", overhead);
#endif

		fprintf(f, "\n\t$TQA_QOS parent 1:%u handle %u: $Q\n"
		           "\t$TFA_QOS parent 1: prio %u protocol ip handle %u fw flowid 1:%u\n",
		           classid, classid,
		           classid, priority, classid);
#ifdef TCONFIG_IPV6
		fprintf(f, "\t$TFA_QOS parent 1: prio %u protocol ipv6 handle %u fw flowid 1:%u\n", (classid + 100), priority, classid);
#endif
	} /* for */
	free(buf);

	/* write commands which adds rule to forward traffic to IFB device */
	fprintf(f, "\n\t# set up the IFB device (otherwise this won't work) to limit the incoming data\n"
	           "\tip link set $QOS_DEV up\n"
	           "\t;;\n"
	           "stop)\n"
	           "\tip link set $QOS_DEV down\n"
	           "\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
	           "\ttc qdisc del dev $QOS_DEV root 2>/dev/null\n");

#ifdef TCONFIG_BCMARM
	fprintf(f, "\ttc filter del dev $WAN_DEV parent ffff: protocol ip prio 10 u32 match ip %s action mirred egress redirect dev $QOS_DEV 2>/dev/null\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst 0.0.0.0/0"));
#ifdef TCONFIG_IPV6
	fprintf(f, "\ttc filter del dev $WAN_DEV parent ffff: protocol ipv6 prio 11 u32 match ip6 %s action mirred egress redirect dev $QOS_DEV 2>/dev/null\n", (nvram_get_int("qos_udp") ? "protocol 6 0xff" : "dst ::/0"));
#endif
#endif /* TCONFIG_BCMARM */

	fprintf(f, "\t;;\n"
	           "*)\n"
	           "\techo \"...\"\n"
	           "\techo \"... OUTGOING QDISCS AND CLASSES FOR $WAN_DEV\"\n"
	           "\techo \"...\"\n"
	           "\ttc -s -d qdisc ls dev $WAN_DEV\n"
	           "\techo\n"
	           "\ttc -s -d class ls dev $WAN_DEV\n"
	           "\techo\n"
	           "\techo \"...\"\n"
	           "\techo \"... INCOMING QDISCS AND CLASSES FOR $WAN_DEV (routed through $QOS_DEV)\"\n"
	           "\techo \"...\"\n"
	           "\ttc -s -d qdisc ls dev $QOS_DEV\n"
	           "\techo\n"
	           "\ttc -s -d class ls dev $QOS_DEV\n"
	           "\techo\n"
	           "esac\n");

	fclose(f);

	chmod(qosfn, 0700);

	eval(qosfn, "start");
}

void stop_qos(char *prefix)
{
	prep_qosstr(prefix);

	eval(qosfn, "stop");
}

/*
 * PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
 *            QD         |                         ^
 *                       |                         |
 *                       v                         |
 *                     INPUT (f)                 OUTPUT (mnf)
 */
