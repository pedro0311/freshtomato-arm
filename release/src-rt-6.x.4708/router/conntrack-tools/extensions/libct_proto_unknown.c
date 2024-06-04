/*
 * (C) 2005-2008 by Pablo Neira Ayuso <pablo@netfilter.org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */
#include <stdio.h>
#include <getopt.h>

#include "conntrack.h"

static struct option opts[] = {
        {0, 0, 0, 0}
};

static void help(void)
{
	fprintf(stdout, "  no options (unsupported)\n");
}

static void final_check(unsigned int flags,
		        unsigned int cmd,
		        struct nf_conntrack *ct)
{
	if (nfct_attr_is_set(ct, ATTR_REPL_L3PROTO) &&
	    nfct_attr_is_set(ct, ATTR_L4PROTO) &&
	    !nfct_attr_is_set(ct, ATTR_REPL_L4PROTO))
		nfct_set_attr_u8(ct, ATTR_REPL_L4PROTO, nfct_get_attr_u8(ct, ATTR_L4PROTO));
}

struct ctproto_handler ct_proto_unknown = {
	.name 		= "unknown",
	.help		= help,
	.opts		= opts,
	.final_check	= final_check,
	.version	= VERSION,
};

void register_unknown(void)
{
	/* we don't actually insert this protocol in the list */
}
