
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include <libnetfilter_log/libnetfilter_log.h>

static int print_pkt(struct nflog_data *nfad)
{
	uint32_t outdev, indev, hw_addrlen;
	struct nfulnl_msg_packet_hdr *ph;
	struct nfulnl_msg_packet_hw *hw;
	int payload_len;
	char *payload;
	char *prefix;
	int i;

	ph = nflog_get_msg_packet_hdr(nfad);
	if (ph) {
		printf("hw_protocol=0x%04x hook=%u ",
			ntohs(ph->hw_protocol), ph->hook);
	}

	hw = nflog_get_packet_hw(nfad);
	if (hw) {
		hw_addrlen = ntohs(hw->hw_addrlen);
		printf("hw_addrlen=%d ", hw_addrlen);

		printf("hw_addr=");
		for (i = 0; i < hw_addrlen - 1; i++)
			printf("%02x:", hw->hw_addr[i]);

		printf("%02x ", hw->hw_addr[hw_addrlen - 1]);
	}

	printf("mark=%u ", nflog_get_nfmark(nfad));

	indev = nflog_get_indev(nfad);
	if (indev > 0)
		printf("indev=%u ", indev);

	outdev = nflog_get_outdev(nfad);
	if (outdev > 0)
		printf("outdev=%u ", outdev);


	prefix = nflog_get_prefix(nfad);
	if (prefix)
		printf("prefix=\"%s\" ", prefix);

	payload_len = nflog_get_payload(nfad, &payload);
	if (payload_len >= 0)
		printf("payload_len=%d ", payload_len);

	fputc('\n', stdout);
	return 0;
}

static int cb(struct nflog_g_handle *gh, struct nfgenmsg *nfmsg,
		struct nflog_data *nfa, void *data)
{
	print_pkt(nfa);
	return 0;
}


int main(int argc, char **argv)
{
	struct nflog_handle *h;
	struct nflog_g_handle *gh;
	struct nflog_g_handle *gh100;
	int rv, fd;
	char buf[4096];

	h = nflog_open();
	if (!h) {
		perror("nflog_open");
		exit(1);
	}

	printf("unbinding existing nf_log handler for AF_INET (if any)\n");
	if (nflog_unbind_pf(h, AF_INET) < 0) {
		perror("nflog_unbind_pf");
		exit(1);
	}

	printf("binding nfnetlink_log to AF_INET\n");
	if (nflog_bind_pf(h, AF_INET) < 0) {
		perror("nflog_bind_pf");
		exit(1);
	}
	printf("binding this socket to group 0\n");
	gh = nflog_bind_group(h, 0);
	if (!gh) {
		perror("nflog_bind_group 0");
		exit(1);
	}

	printf("binding this socket to group 100\n");
	gh100 = nflog_bind_group(h, 100);
	if (!gh100) {
		perror("nflog_bind_group 100");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nflog_set_mode(gh, NFULNL_COPY_PACKET, 0xffff) < 0) {
		perror("nflog_set_mode NFULNL_COPY_PACKET");
		exit(1);
	}

	fd = nflog_fd(h);

	printf("registering callback for group 0\n");
	nflog_callback_register(gh, &cb, NULL);

	printf("going into main loop\n");
	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received (len=%u)\n", rv);

		/* handle messages in just-received packet */
		nflog_handle_packet(h, buf, rv);
	}

	printf("unbinding from group 100\n");
	nflog_unbind_group(gh100);
	printf("unbinding from group 0\n");
	nflog_unbind_group(gh);

#ifdef INSANE
	/* norally, applications SHOULD NOT issue this command,
	 * since it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nflog_unbind_pf(h, AF_INET);
#endif

	printf("closing handle\n");
	nflog_close(h);

	return EXIT_SUCCESS;
}
