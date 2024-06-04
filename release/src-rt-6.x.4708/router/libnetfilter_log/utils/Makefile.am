include ${top_srcdir}/Make_global.am

check_PROGRAMS = nfulnl_test nf-log

nfulnl_test_SOURCES = nfulnl_test.c
nfulnl_test_LDADD = ../src/libnetfilter_log.la

nf_log_SOURCES  = nf-log.c
nf_log_LDADD    = ../src/libnetfilter_log.la $(LIBMNL_LIBS)
nf_log_CPPFLAGS = $(AM_CPPFLAGS) $(LIBMNL_CFLAGS)
if BUILD_NFCT
nf_log_LDADD    += $(LIBNETFILTER_CONNTRACK_LIBS)
nf_log_CPPFLAGS += $(LIBNETFILTER_CONNTRACK_CFLAGS) -DBUILD_NFCT
endif

if BUILD_IPULOG
check_PROGRAMS += ulog_test

ulog_test_SOURCES = ulog_test.c
ulog_test_LDADD   = ../src/libnetfilter_log_libipulog.la
endif
