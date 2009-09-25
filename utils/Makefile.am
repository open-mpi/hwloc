# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

include $(top_srcdir)/hwloc.am
include $(top_srcdir)/Makefile.man-page-rules

EXTRA_DIST = lstopo.h hwloc-mask.h test-hwloc-distrib.output

bin_PROGRAMS = lstopo hwloc-mask hwloc-bind hwloc-distrib

lstopo_SOURCES = lstopo.c lstopo-color.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c lstopo-xml.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
endif
lstopo_CFLAGS = $(CAIRO_CFLAGS) $(XML_CFLAGS)
lstopo_LDADD = $(CAIRO_LIBS) $(XML_LIBS) -lm

LIBS = $(top_builddir)/src/libhwloc.la

if HAVE_MINGW32
else
TESTS = test-hwloc-distrib.sh
endif

man_pages = lstopo.1 hwloc-bind.1 hwloc-distrib.1 hwloc-mask.1
EXTRA_DIST += $(man_pages:.1=.1in)
nodist_man_MANS = $(man_pages)

install-exec-hook:
	$(RM) $(DESTDIR)$(bindir)/hwloc-ls $(DESTDIR)$(bindir)/hwloc-info
	$(LN_S) lstopo $(DESTDIR)$(bindir)/hwloc-ls
	$(LN_S) lstopo $(DESTDIR)$(bindir)/hwloc-info

uninstall-local:
	$(RM) $(DESTDIR)$(bindir)/hwloc-ls 
	$(RM) $(DESTDIR)$(bindir)/hwloc-info

distclean-local:
	rm -f $(man_pages)
