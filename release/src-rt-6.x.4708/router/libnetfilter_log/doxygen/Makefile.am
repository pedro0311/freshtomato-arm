if HAVE_DOXYGEN

doc_srcs = $(top_srcdir)/src/libnetfilter_log.c\
	   $(top_srcdir)/src/nlmsg.c\
	   $(top_srcdir)/src/libipulog_compat.c

doxyfile.stamp: $(doc_srcs) Makefile
	rm -rf html man
	doxygen doxygen.cfg >/dev/null

if BUILD_MAN
	$(abs_top_srcdir)/doxygen/build_man.sh
endif

	touch doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp
clean-local:
	rm -rf man html
install-data-local:
if BUILD_MAN
	mkdir -p $(DESTDIR)$(mandir)/man3
	cp --no-dereference --preserve=links,mode,timestamps man/man3/*.3\
	  $(DESTDIR)$(mandir)/man3/
endif
if BUILD_HTML
	mkdir  -p $(DESTDIR)$(htmldir)
	cp  --no-dereference --preserve=links,mode,timestamps html/*\
		$(DESTDIR)$(htmldir)
endif

# make distcheck needs uninstall-local
uninstall-local:
	rm -rf $(DESTDIR)$(mandir) man html doxyfile.stamp $(DESTDIR)$(htmldir)
endif

EXTRA_DIST = build_man.sh
