dnl Process this file with autoconf to create configure.

AC_INIT([libnetfilter_log], [1.0.2])
AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall foreign subdir-objects
	tar-pax no-dist-gzip dist-bzip2 1.6])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl kernel style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([html-doc],
	      AS_HELP_STRING([--enable-html-doc], [Enable html documentation]),
	      [], [enable_html_doc=no])
AM_CONDITIONAL([BUILD_HTML], [test "$enable_html_doc" = yes])
AS_IF([test "$enable_html_doc" = yes],
      [AC_SUBST(GEN_HTML, YES)],
      [AC_SUBST(GEN_HTML, NO)])

AC_ARG_ENABLE([man-pages],
	      AS_HELP_STRING([--disable-man-pages],
			     [Disable man page documentation]),
	      [], [enable_man_pages=yes])
AM_CONDITIONAL([BUILD_MAN], [test "$enable_man_pages" = yes])
AS_IF([test "$enable_man_pages" = yes],
      [AC_SUBST(GEN_MAN, YES)],
      [AC_SUBST(GEN_MAN, NO)])

AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT([disable_static])
AC_PROG_INSTALL
AC_PROG_LN_S

case "$host" in
*-*-linux* | *-*-uclinux*) ;;
*) AC_MSG_ERROR([Linux only, dude!]);;
esac

AC_ARG_WITH([ipulog],
  AS_HELP_STRING([--without-ipulog], [don't build libipulog compat library]))
AM_CONDITIONAL([BUILD_IPULOG], [test "x$with_ipulog" != xno])

dnl Dependencies
AC_SUBST([LIBNFNETLINK_MIN_VERSION], [0.0.41])
AC_SUBST([LIBMNL_MIN_VERSION],       [1.0.3])

PKG_CHECK_MODULES([LIBNFNETLINK], [libnfnetlink >= ${LIBNFNETLINK_MIN_VERSION}])
PKG_CHECK_MODULES([LIBMNL], [libmnl >= ${LIBMNL_MIN_VERSION}])
PKG_CHECK_MODULES([LIBNETFILTER_CONNTRACK], [libnetfilter_conntrack >= 1.0.2],
		  [HAVE_LNFCT=1], [HAVE_LNFCT=0])
AM_CONDITIONAL([BUILD_NFCT], [test "$HAVE_LNFCT" -eq 1])

AS_IF([test "$enable_man_pages" = no -a "$enable_html_doc" = no],
      [with_doxygen=no], [with_doxygen=yes])

AS_IF([test "x$with_doxygen" != xno], [
	AC_CHECK_PROGS([DOXYGEN], [doxygen], [""])
	AC_CHECK_PROGS([DOT], [dot], [""])
	AS_IF([test "x$DOT" != "x"],
	      [AC_SUBST(HAVE_DOT, YES)],
	      [AC_SUBST(HAVE_DOT, NO)])
])

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AS_IF([test "x$DOXYGEN" = x], [
	AS_IF([test "x$with_doxygen" != xno], [
		dnl Only run doxygen Makefile if doxygen installed
		AC_MSG_WARN([Doxygen not found - not building documentation])
		enable_html_doc=no
		enable_man_pages=no
	])
])

dnl Output the makefile
AC_CONFIG_FILES([Makefile
		src/Makefile
		include/Makefile
		include/libnetfilter_log/Makefile
		utils/Makefile
		libnetfilter_log.pc
		libnetfilter_log_libipulog.pc
		doxygen/Makefile
		doxygen/doxygen.cfg])
AC_OUTPUT

echo "
libnetfilter_log configuration:
man pages:                    ${enable_man_pages}
html docs:                    ${enable_html_doc}"
