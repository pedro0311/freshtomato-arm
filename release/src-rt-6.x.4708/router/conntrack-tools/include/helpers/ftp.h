#ifndef _CTD_FTP_H
#define _CTD_FTP_H

#define NUM_SEQ_TO_REMEMBER 2

/* This structure exists only once per master */
struct ftp_info {
	/* Valid seq positions for cmd matching after newline */
	uint32_t seq_aft_nl[MYCT_DIR_MAX][NUM_SEQ_TO_REMEMBER];
	/* 0 means seq_match_aft_nl not set */
	int seq_aft_nl_num[MYCT_DIR_MAX];
};

#endif
