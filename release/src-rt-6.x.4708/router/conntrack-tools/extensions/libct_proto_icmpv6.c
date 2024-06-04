/*
 * (C) 2008 by Krzysztof Piotr Oledzki <ole@ans.pl>
 *
 * Based on libct_proto_icmp.c:
 * (C) 2005-2007 by Pablo Neira Ayuso <pablo@netfilter.org>
 *     2005 by Harald Welte <laforge@netfilter.org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */

#include "conntrack.h"

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <netinet/in.h> /* For htons */
#include <netinet/icmp6.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>

enum {
	CT_ICMP_TYPE	= (1 << 0),
	CT_ICMP_CODE	= (1 << 1),
	CT_ICMP_ID	= (1 << 2),
};

static struct option opts[] = {
	{ "icmpv6-type", 1, 0, '1' },
	{ "icmpv6-code", 1, 0, '2' },
	{ "icmpv6-id",  1, 0, '3' },
	{ 0, 0, 0, 0 },
};

#define ICMPV6_NUMBER_OF_OPT 4

static const char *icmpv6_optflags[ICMPV6_NUMBER_OF_OPT] = {
	"icmpv6-type", "icmpv6-code", "icmpv6-id"
};

static char icmpv6_commands_v_options[NUMBER_OF_CMD][ICMPV6_NUMBER_OF_OPT] =
/* Well, it's better than "Re: Maradona vs Pele" */
{
				/* 1 2 3 */
	[CT_LIST_BIT]		= {2,2,2},
	[CT_CREATE_BIT]		= {1,1,2},
	[CT_UPDATE_BIT]		= {2,2,2},
	[CT_DELETE_BIT]		= {2,2,2},
	[CT_GET_BIT]		= {1,1,2},
	[CT_FLUSH_BIT]		= {0,0,0},
	[CT_EVENT_BIT]		= {2,2,2},
	[CT_VERSION_BIT]	= {0,0,0},
	[CT_HELP_BIT]		= {0,0,0},
	[EXP_LIST_BIT]		= {0,0,0},
	[EXP_CREATE_BIT]	= {0,0,0},
	[EXP_DELETE_BIT]	= {0,0,0},
	[EXP_GET_BIT]		= {0,0,0},
	[EXP_FLUSH_BIT]		= {0,0,0},
	[EXP_EVENT_BIT]		= {0,0,0},
	[CT_ADD_BIT]		= {1,1,2},
};

static void help(void)
{
	fprintf(stdout, "  --icmpv6-type\t\t\ticmpv6 type\n");
	fprintf(stdout, "  --icmpv6-code\t\t\ticmpv6 code\n");
	fprintf(stdout, "  --icmpv6-id\t\t\ticmpv6 id\n");
}

static int parse(char c,
		 struct nf_conntrack *ct,
		 struct nf_conntrack *exptuple,
		 struct nf_conntrack *mask,
		 unsigned int *flags)
{
	switch(c) {
		uint8_t tmp;
		uint16_t id;
		case '1':
			tmp = atoi(optarg);
			nfct_set_attr_u8(ct, ATTR_ICMP_TYPE, tmp);
			nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_ICMPV6);
			if (nfct_attr_is_set(ct, ATTR_REPL_L3PROTO))
				nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, IPPROTO_ICMPV6);
			*flags |= CT_ICMP_TYPE;
			break;
		case '2':
			tmp = atoi(optarg);
			nfct_set_attr_u8(ct, ATTR_ICMP_CODE, tmp);
			nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_ICMPV6);
			if (nfct_attr_is_set(ct, ATTR_REPL_L3PROTO))
				nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, IPPROTO_ICMPV6);
			*flags |= CT_ICMP_CODE;
			break;
		case '3':
			id = htons(atoi(optarg));
			nfct_set_attr_u16(ct, ATTR_ICMP_ID, id);
			nfct_set_attr_u8(ct, ATTR_L4PROTO, IPPROTO_ICMPV6);
			if (nfct_attr_is_set(ct, ATTR_REPL_L3PROTO))
				nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, IPPROTO_ICMPV6);
			*flags |= CT_ICMP_ID;
			break;
	}
	return 1;
}

static const struct ct_print_opts icmpv6_print_opts[] = {
	{"--icmpv6-type", ATTR_ICMP_TYPE, CT_ATTR_TYPE_U8, 0, 0},
	{"--icmpv6-code", ATTR_ICMP_CODE, CT_ATTR_TYPE_U8, 0, 0},
	{"--icmpv6-id", ATTR_ICMP_ID, CT_ATTR_TYPE_BE16, 0, 0},
	{},
};

static void final_check(unsigned int flags,
		        unsigned int cmd,
		        struct nf_conntrack *ct)
{
	generic_opt_check(flags, ICMPV6_NUMBER_OF_OPT,
			  icmpv6_commands_v_options[cmd], icmpv6_optflags,
			  NULL, 0, NULL);
}

static struct ctproto_handler icmpv6 = {
	.name 		= "icmpv6",
	.protonum	= IPPROTO_ICMPV6,
	.parse_opts	= parse,
	.final_check	= final_check,
	.print_opts	= icmpv6_print_opts,
	.help		= help,
	.opts		= opts,
	.version	= VERSION,
};

void register_icmpv6(void)
{
	register_proto(&icmpv6);
}
