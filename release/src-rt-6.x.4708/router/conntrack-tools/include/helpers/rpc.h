#ifndef _CTD_RPC_H
#define _CTD_RPC_H

struct rpc_info {
	/* XID */
	uint32_t xid;
	/* program */
	uint32_t pm_prog;
	/* program version */
	uint32_t pm_vers;
	/* transport protocol: TCP|UDP */
	uint32_t pm_prot;
};

#endif
