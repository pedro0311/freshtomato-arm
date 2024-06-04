include $(top_srcdir)/Make_global.am

if HAVE_CTHELPER
SUBDIRS = helpers
endif

AM_YFLAGS = -d

BUILT_SOURCES = read_config_yy.h

sbin_PROGRAMS = conntrack conntrackd nfct

conntrack_SOURCES = conntrack.c
conntrack_LDADD   = ../extensions/libct_proto_dccp.la    \
                    ../extensions/libct_proto_gre.la     \
                    ../extensions/libct_proto_icmp.la    \
                    ../extensions/libct_proto_icmpv6.la  \
                    ../extensions/libct_proto_sctp.la    \
                    ../extensions/libct_proto_tcp.la     \
                    ../extensions/libct_proto_udp.la     \
                    ../extensions/libct_proto_udplite.la \
                    ../extensions/libct_proto_unknown.la \
                    ${LIBMNL_LIBS}                       \
                    ${LIBNETFILTER_CONNTRACK_LIBS}       \
                    ${LIBNFNETLINK_LIBS}

nfct_SOURCES = nfct.c

if HAVE_CTHELPER
nfct_SOURCES += helpers.c			\
		nfct-extensions/helper.c
endif

if HAVE_CTTIMEOUT
nfct_SOURCES += nfct-extensions/timeout.c
endif

nfct_LDADD = ${LIBMNL_LIBS} 			\
	     ${libdl_LIBS}

if HAVE_CTTIMEOUT
nfct_LDADD += ${LIBNETFILTER_CTTIMEOUT_LIBS}
endif

if HAVE_CTHELPER
nfct_LDADD += ${LIBNETFILTER_CTHELPER_LIBS}
endif

conntrackd_SOURCES = alarm.c            \
                     build.c            \
                     cache.c            \
                     cache-ct.c         \
                     cache-exp.c        \
                     cache_timer.c      \
                     channel.c          \
                     channel_mcast.c    \
                     channel_tcp.c      \
                     channel_udp.c      \
                     cidr.c             \
                     ctnl.c             \
                     date.c             \
                     event.c            \
                     external_cache.c   \
                     external_inject.c  \
                     fds.c              \
                     filter.c           \
                     hash.c             \
                     internal_bypass.c  \
                     internal_cache.c   \
                     local.c            \
                     log.c              \
                     main.c             \
                     mcast.c            \
                     multichannel.c     \
                     netlink.c          \
                     network.c          \
                     origin.c           \
                     parse.c            \
                     process.c          \
                     queue.c            \
                     queue_tx.c         \
                     rbtree.c           \
                     read_config_lex.l  \
                     read_config_yy.y   \
                     resync.c           \
                     run.c              \
                     stack.c            \
                     stats-mode.c       \
                     sync-alarm.c       \
                     sync-ftfw.c        \
                     sync-mode.c        \
                     sync-notrack.c     \
                     tcp.c              \
                     traffic_stats.c    \
                     udp.c              \
                     vector.c

if HAVE_CTHELPER
conntrackd_SOURCES += cthelper.c helpers.c utils.c expect.c
endif

if HAVE_SYSTEMD
conntrackd_SOURCES += systemd.c
endif

conntrackd_LDADD = ${LIBMNL_LIBS} ${LIBNETFILTER_CONNTRACK_LIBS} \
		   ${libdl_LIBS} ${LIBNFNETLINK_LIBS}

if HAVE_CTHELPER
conntrackd_LDADD += ${LIBNETFILTER_CTHELPER_LIBS} ${LIBNETFILTER_QUEUE_LIBS}
endif

if HAVE_SYSTEMD
conntrackd_LDADD += ${LIBSYSTEMD_LIBS}
endif

conntrackd_LDFLAGS = -export-dynamic
