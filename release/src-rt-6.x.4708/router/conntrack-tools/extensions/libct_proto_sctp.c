/*
 * (C) 2009 by Pablo Neira Ayuso <pablo@netfilter.org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> /* For htons */
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack_sctp.h>

#include "conntrack.h"

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

enum {
	CT_SCTP_ORIG_SPORT	= (1 << 0),
	CT_SCTP_ORIG_DPORT	= (1 << 1),
	CT_SCTP_REPL_SPORT	= (1 << 2),
	CT_SCTP_REPL_DPORT	= (1 << 3),
	CT_SCTP_MASK_SPORT	= (1 << 4),
	CT_SCTP_MASK_DPORT	= (1 << 5),
	CT_SCTP_STATE		= (1 << 6),
	CT_SCTP_EXPTUPLE_SPORT	= (1 << 7),
	CT_SCTP_EXPTUPLE_DPORT	= (1 << 8),
	CT_SCTP_ORIG_VTAG	= (1 << 9),
	CT_SCTP_REPL_VTAG	= (1 << 10),
};

#define SCTP_OPT_MAX	14
static struct option opts[SCTP_OPT_MAX] = {
	{ .name = "orig-port-src",	.has_arg = 1, .val = 1 },
	{ .name = "orig-port-dst",	.has_arg = 1, .val = 2 },
	{ .name = "reply-port-src",	.has_arg = 1, .val = 3 },
	{ .name = "reply-port-dst",	.has_arg = 1, .val = 4 },
	{ .name = "mask-port-src",	.has_arg = 1, .val = 5 },
	{ .name = "mask-port-dst",	.has_arg = 1, .val = 6 },
	{ .name = "state",		.has_arg = 1, .val = 7 },
	{ .name = "tuple-port-src",	.has_arg = 1, .val = 8 },
	{ .name = "tuple-port-dst",	.has_arg = 1, .val = 9 },
	{ .name = "orig-vtag",		.has_arg = 1, .val = 10 },
	{ .name = "reply-vtag",		.has_arg = 1, .val = 11 },
	{ .name = "sport",		.has_arg = 1, .val = 1 }, /* alias */
	{ .name = "dport",		.has_arg = 1, .val = 2 }, /* alias */
	{0, 0, 0, 0}
};

static const char *sctp_optflags[SCTP_OPT_MAX] = {
	[0] = "sport",
	[1] = "dport",
	[2] = "reply-port-src",
	[3] = "reply-port-dst",
	[4] = "mask-port-src",
	[5] = "mask-port-dst",
	[6] = "state",
	[7] = "tuple-port-src",
	[8] = "tuple-port-dst",
	[9] = "orig-vtag",
	[10] = "reply-vtag",
};

static char sctp_commands_v_options[NUMBER_OF_CMD][SCTP_OPT_MAX] =
/* Well, it's better than "Re: Sevilla vs Betis" */
{
				/* 1 2 3 4 5 6 7 8 9 10 11 */
	[CT_LIST_BIT]		= {2,2,2,2,0,0,2,0,0,0,0},
	[CT_CREATE_BIT]		= {3,3,3,3,0,0,1,0,0,1,1},
	[CT_UPDATE_BIT]		= {2,2,2,2,0,0,2,0,0,2,2},
	[CT_DELETE_BIT]		= {2,2,2,2,0,0,2,0,0,0,0},
	[CT_GET_BIT]		= {3,3,3,3,0,0,2,0,0,2,2},
	[CT_FLUSH_BIT]		= {0,0,0,0,0,0,0,0,0,0,0},
	[CT_EVENT_BIT]		= {2,2,2,2,0,0,2,0,0,0,0},
	[CT_VERSION_BIT]	= {0,0,0,0,0,0,0,0,0,0,0},
	[CT_HELP_BIT]		= {0,0,0,0,0,0,0,0,0,0,0},
	[EXP_LIST_BIT]		= {0,0,0,0,0,0,0,0,0,0,0},
	[EXP_CREATE_BIT]	= {1,1,0,0,1,1,0,1,1,1,1},
	[EXP_DELETE_BIT]	= {1,1,1,1,0,0,0,0,0,0,0},
	[EXP_GET_BIT]		= {1,1,1,1,0,0,0,0,0,0,0},
	[EXP_FLUSH_BIT]		= {0,0,0,0,0,0,0,0,0,0,0},
	[EXP_EVENT_BIT]		= {0,0,0,0,0,0,0,0,0,0,0},
	[CT_ADD_BIT]		= {3,3,3,3,0,0,1,0,0,1,1},
};

static const char *sctp_states[SCTP_CONNTRACK_MAX] = {
	[SCTP_CONNTRACK_NONE]			= "NONE",
	[SCTP_CONNTRACK_CLOSED]			= "CLOSED",
	[SCTP_CONNTRACK_COOKIE_WAIT]		= "COOKIE_WAIT",
	[SCTP_CONNTRACK_COOKIE_ECHOED]		= "COOKIE_ECHOED",
	[SCTP_CONNTRACK_ESTABLISHED]		= "ESTABLISHED",
	[SCTP_CONNTRACK_SHUTDOWN_SENT]		= "SHUTDOWN_SENT",
	[SCTP_CONNTRACK_SHUTDOWN_RECD]		= "SHUTDOWN_RECD",
	[SCTP_CONNTRACK_SHUTDOWN_ACK_SENT]	= "SHUTDOWN_ACK_SENT",
};

static void help(void)
{
	fprintf(stdout, "  --orig-port-src\t\toriginal source port\n");
	fprintf(stdout, "  --orig-port-dst\t\toriginal destination port\n");
	fprintf(stdout, "  --reply-port-src\t\treply source port\n");
	fprintf(stdout, "  --reply-port-dst\t\treply destination port\n");
	fprintf(stdout, "  --mask-port-src\t\tmask source port\n");
	fprintf(stdout, "  --mask-port-dst\t\tmask destination port\n");
	fprintf(stdout, "  --tuple-port-src\t\texpectation tuple src port\n");
	fprintf(stdout, "  --tuple-port-src\t\texpectation tuple dst port\n");
	fprintf(stdout, "  --state\t\t\tSCTP state, fe. ESTABLISHED\n");
	fprintf(stdout, "  --orig-vtag\t\toriginal verification tag\n");
	fprintf(stdout, "  --reply-vtag\t\treply verification tag\n");
}

static int
parse_options(char c, struct nf_conntrack *ct,
	      struct nf_conntrack *exptuple, struct nf_conntrack *mask,
	      unsigned int *flags)
{
	int i;
	uint16_t port;
	uint32_t vtag;

	switch(c) {
	case 1:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(ct, ATTR_ORIG_PORT_SRC, port);
		nfct_set_attr_u8(ct, ATTR_ORIG_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_ORIG_SPORT;
		break;
	case 2:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(ct, ATTR_ORIG_PORT_DST, port);
		nfct_set_attr_u8(ct, ATTR_ORIG_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_ORIG_DPORT;
		break;
	case 3:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(ct, ATTR_REPL_PORT_SRC, port);
		nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_REPL_SPORT;
		break;
	case 4:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(ct, ATTR_REPL_PORT_DST, port);
		nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_REPL_DPORT;
		break;
	case 5:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(mask, ATTR_ORIG_PORT_SRC, port);
		nfct_set_attr_u8(mask, ATTR_ORIG_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_MASK_SPORT;
		break;
	case 6:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(mask, ATTR_ORIG_PORT_DST, port);
		nfct_set_attr_u8(mask, ATTR_ORIG_L4PROTO, IPPROTO_SCTP);
		*flags |= CT_SCTP_MASK_DPORT;
		break;
	case 7:
		for (i=0; i<SCTP_CONNTRACK_MAX; i++) {
			if (strcmp(optarg, sctp_states[i]) == 0) {
				nfct_set_attr_u8(ct, ATTR_SCTP_STATE, i);
				break;
			}
		}
		if (i == SCTP_CONNTRACK_MAX)
			exit_error(PARAMETER_PROBLEM,
				   "unknown SCTP state `%s'", optarg);
		*flags |= CT_SCTP_STATE;
		break;
	case 8:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(exptuple, ATTR_ORIG_PORT_SRC, port);
		nfct_set_attr_u8(exptuple, ATTR_ORIG_L4PROTO, port);
		*flags |= CT_SCTP_EXPTUPLE_SPORT;
		break;
	case 9:
		port = htons(atoi(optarg));
		nfct_set_attr_u16(exptuple, ATTR_ORIG_PORT_DST, port); 
		nfct_set_attr_u8(exptuple, ATTR_ORIG_L4PROTO, port);
		*flags |= CT_SCTP_EXPTUPLE_DPORT;
		break;
	case 10:
		vtag = htonl(atoi(optarg));
		nfct_set_attr_u32(ct, ATTR_SCTP_VTAG_ORIG, vtag); 
		*flags |= CT_SCTP_ORIG_VTAG;
		break;
	case 11:
		vtag = htonl(atoi(optarg));
		nfct_set_attr_u32(ct, ATTR_SCTP_VTAG_REPL, vtag); 
		*flags |= CT_SCTP_REPL_VTAG;
		break;
	}
	return 1;
}

static const struct ct_print_opts sctp_print_opts[] = {
	{ "--sport", ATTR_ORIG_PORT_SRC, CT_ATTR_TYPE_BE16, 0, 0 },
	{ "--dport", ATTR_ORIG_PORT_DST, CT_ATTR_TYPE_BE16, 0, 0 },
	{ "--reply-port-src", ATTR_REPL_PORT_SRC, CT_ATTR_TYPE_BE16, 0, 0 },
	{ "--reply-port-dst", ATTR_REPL_PORT_DST, CT_ATTR_TYPE_BE16, 0, 0 },
	{ "--state", ATTR_SCTP_STATE, CT_ATTR_TYPE_U8, SCTP_CONNTRACK_MAX, sctp_states },
	{ "--orig-vtag", ATTR_SCTP_VTAG_ORIG, CT_ATTR_TYPE_BE32, 0, 0 },
	{ "--reply-vtag", ATTR_SCTP_VTAG_REPL, CT_ATTR_TYPE_BE32, 0, 0 },
	{},
};

#define SCTP_VALID_FLAGS_MAX   2
static unsigned int dccp_valid_flags[SCTP_VALID_FLAGS_MAX] = {
	CT_SCTP_ORIG_SPORT | CT_SCTP_ORIG_DPORT,
	CT_SCTP_REPL_SPORT | CT_SCTP_REPL_DPORT,
};

static void
final_check(unsigned int flags, unsigned int cmd, struct nf_conntrack *ct)
{
	int ret, partial;

	ret = generic_opt_check(flags, SCTP_OPT_MAX,
				sctp_commands_v_options[cmd], sctp_optflags,
				dccp_valid_flags, SCTP_VALID_FLAGS_MAX,
				&partial);
	if (!ret) {
		switch(partial) {
		case -1:
		case 0:
			exit_error(PARAMETER_PROBLEM, "you have to specify "
						      "`--sport' and "
						      "`--dport'");
			break;
		case 1:
			exit_error(PARAMETER_PROBLEM, "you have to specify "
						      "`--reply-src-port' and "
						      "`--reply-dst-port'");
			break;
		}
	}
}

static struct ctproto_handler sctp = {
	.name 			= "sctp",
	.protonum		= IPPROTO_SCTP,
	.parse_opts		= parse_options,
	.final_check		= final_check,
	.print_opts		= sctp_print_opts,
	.help			= help,
	.opts			= opts,
	.version		= VERSION,
};

void register_sctp(void)
{
	register_proto(&sctp);
}
