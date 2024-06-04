/* This example is placed in the public domain. */
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <libnetfilter_log/linux_nfnetlink_log.h>

#include <libmnl/libmnl.h>
#include <libnetfilter_log/libnetfilter_log.h>

#ifdef BUILD_NFCT
#include <linux/netfilter/nf_conntrack_common.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>
#endif

#ifdef BUILD_NFCT
static int print_ctinfo(const struct nlattr *const attr)
{
	uint32_t ctinfo;

	if (attr == NULL)
		return MNL_CB_OK;

	ctinfo = ntohl(mnl_attr_get_u32(attr));
	printf("  ip_conntrack_info:");

	switch (CTINFO2DIR(ctinfo)) {
	case IP_CT_DIR_ORIGINAL:
		printf(" ORIGINAL /");
		break;
	case IP_CT_DIR_REPLY:
		printf(" REPLY /");
		break;
	default:
		printf(" unknown dir: %d\n", CTINFO2DIR(ctinfo));
		return MNL_CB_ERROR;
	}

	switch (ctinfo) {
	case IP_CT_ESTABLISHED:
	case IP_CT_ESTABLISHED_REPLY:
		printf(" ESTABLISHED\n");
		break;
	case IP_CT_RELATED:
	case IP_CT_RELATED_REPLY:
		printf(" RELATED\n");
		break;
	case IP_CT_NEW:
	case IP_CT_NEW_REPLY:
		printf(" NEW\n");
		break;
	default:
		printf(" unknown ctinfo: %d\n", ctinfo);
		return MNL_CB_ERROR;
	}

	return MNL_CB_OK;
}

static int print_nfct(uint8_t family,
		      const struct nlattr *const info_attr,
		      const struct nlattr *const ct_attr)
{
	char buf[4096];
	struct nf_conntrack *ct = NULL;

	if (info_attr != NULL)
		print_ctinfo(info_attr);

	if (ct_attr == NULL)
		return MNL_CB_OK;

	ct = nfct_new();
	if (ct == NULL) {
		perror("nfct_new");
		return MNL_CB_ERROR;
	}

	if (nfct_payload_parse(mnl_attr_get_payload(ct_attr),
			       mnl_attr_get_payload_len(ct_attr),
			       family, ct) < 0) {
		perror("nfct_payload_parse");
		nfct_destroy(ct);
		return MNL_CB_ERROR;
	}

	nfct_snprintf(buf, sizeof(buf), ct, 0, NFCT_O_DEFAULT, 0);
	printf("  %s\n", buf);
	nfct_destroy(ct);

	return MNL_CB_OK;
}
#else
static int print_nfct(uint8_t family,
		      const struct nlattr *const info_attr,
		      const struct nlattr *const ct_attr)
{
	return MNL_CB_OK;
}
#endif

static int log_cb(const struct nlmsghdr *nlh, void *data)
{
	struct nlattr *attrs[NFULA_MAX + 1] = { NULL };
	struct nfulnl_msg_packet_hdr *ph = NULL;
	struct nfgenmsg *nfg;
	const char *prefix = NULL;
	uint32_t mark = 0;
	char buf[4096];
	int ret;

	ret = nflog_nlmsg_parse(nlh, attrs);
	if (ret != MNL_CB_OK)
		return ret;

	nfg = mnl_nlmsg_get_payload(nlh);

	if (attrs[NFULA_PACKET_HDR])
		ph = mnl_attr_get_payload(attrs[NFULA_PACKET_HDR]);
	if (attrs[NFULA_PREFIX])
		prefix = mnl_attr_get_str(attrs[NFULA_PREFIX]);
	if (attrs[NFULA_MARK])
		mark = ntohl(mnl_attr_get_u32(attrs[NFULA_MARK]));

	printf("log received (prefix=\"%s\" hw=0x%04x hook=%u mark=%u)\n",
		prefix ? prefix : "", ntohs(ph->hw_protocol), ph->hook,
		mark);

	ret = nflog_nlmsg_snprintf(buf, sizeof(buf), nlh, attrs,
				   NFLOG_OUTPUT_XML, NFLOG_XML_ALL);
	if (ret < 0)
		return MNL_CB_ERROR;
	printf("%s (ret=%d)\n", buf, ret);

	print_nfct(nfg->nfgen_family, attrs[NFULA_CT_INFO], attrs[NFULA_CT]);

	return MNL_CB_OK;
}

int main(int argc, char *argv[])
{
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	int ret;
	unsigned int portid, gnum;

	if (argc != 2) {
		printf("Usage: %s [queue_num]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	gnum = atoi(argv[1]);

	nl = mnl_socket_open(NETLINK_NETFILTER);
	if (nl == NULL) {
		perror("mnl_socket_open");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		perror("mnl_socket_bind");
		exit(EXIT_FAILURE);
	}
	portid = mnl_socket_get_portid(nl);

	/* kernels 3.8 and later is required to omit PF_(UN)BIND */

	nlh = nflog_nlmsg_put_header(buf, NFULNL_MSG_CONFIG, AF_INET, 0);
	if (nflog_attr_put_cfg_cmd(nlh, NFULNL_CFG_CMD_PF_UNBIND) < 0) {
		perror("nflog_attr_put_cfg_cmd");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	nlh = nflog_nlmsg_put_header(buf, NFULNL_MSG_CONFIG, AF_INET, 0);
	if (nflog_attr_put_cfg_cmd(nlh, NFULNL_CFG_CMD_PF_BIND) < 0) {
		perror("nflog_attr_put_cfg_cmd");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	nlh = nflog_nlmsg_put_header(buf, NFULNL_MSG_CONFIG, AF_INET, gnum);
	if (nflog_attr_put_cfg_cmd(nlh, NFULNL_CFG_CMD_BIND) < 0) {
		perror("nflog_attr_put_cfg_cmd");
		exit(EXIT_FAILURE);
	}

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	nlh = nflog_nlmsg_put_header(buf, NFULNL_MSG_CONFIG, AF_UNSPEC, gnum);
	if (nflog_attr_put_cfg_mode(nlh, NFULNL_COPY_PACKET, 0xffff) < 0) {
		perror("nflog_attr_put_cfg_mode");
		exit(EXIT_FAILURE);
	}

#ifdef BUILD_NFCT
	mnl_attr_put_u16(nlh, NFULA_CFG_FLAGS, htons(NFULNL_CFG_F_CONNTRACK));
#endif

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		perror("mnl_socket_sendto");
		exit(EXIT_FAILURE);
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret == -1) {
		perror("mnl_socket_recvfrom");
		exit(EXIT_FAILURE);
	}
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, 0, portid, log_cb, NULL);
		if (ret < 0){
			perror("mnl_cb_run");
			exit(EXIT_FAILURE);
		}

		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
		if (ret == -1) {
			perror("mnl_socket_recvfrom");
			exit(EXIT_FAILURE);
		}
	}

	mnl_socket_close(nl);

	return 0;
}
