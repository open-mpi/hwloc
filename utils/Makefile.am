# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

include $(top_srcdir)/hwloc.am
include $(top_srcdir)/Makefile.man-page-rules

LIBS = $(top_builddir)/src/libhwloc.la

EXTRA_DIST = test-hwloc-distrib.output

bin_PROGRAMS = lstopo hwloc-mask hwloc-bind hwloc-distrib

lstopo_SOURCES = lstopo.h lstopo.c lstopo-color.c lstopo-text.c lstopo-draw.c lstopo-fig.c
if HAVE_CAIRO
lstopo_SOURCES += lstopo-cairo.c
endif
if HAVE_XML
lstopo_SOURCES += lstopo-xml.c
endif
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
endif
lstopo_CFLAGS = $(CAIRO_CFLAGS) $(XML_CFLAGS)
lstopo_LDADD = $(CAIRO_LIBS) $(XML_LIBS) -lm

hwloc_mask_SOURCES = hwloc-mask.c hwloc-mask.h

if HAVE_MINGW32
else
TESTS = test-hwloc-distrib.sh
endif

man_pages = lstopo.1 hwloc-bind.1 hwloc-distrib.1 hwloc-mask.1
EXTRA_DIST += $(man_pages:.1=.1in)
nodist_man_MANS = $(man_pages)

install-exec-hook: install-man
	rm -f $(DESTDIR)$(bindir)/hwloc-ls$(EXEEXT) $(DESTDIR)$(bindir)/hwloc-info$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) lstopo$(EXEEXT) hwloc-ls$(EXEEXT) && $(LN_S) lstopo$(EXEEXT) hwloc-info$(EXEEXT)
	rm -f $(DESTDIR)$(man1dir)/hwloc-ls.1 $(DESTDIR)$(man1dir)/hwloc-info.1
	cd $(DESTDIR)$(man1dir) && $(LN_S) lstopo.1 hwloc-ls.1 && $(LN_S) lstopo.1 hwloc-info.1

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/hwloc-ls$(EXEEXT) $(DESTDIR)$(bindir)/hwloc-info$(EXEEXT)
	rm -f $(DESTDIR)$(man1dir)/hwloc-ls.1 $(DESTDIR)$(man1dir)/hwloc-info.1

distclean-local:
	rm -f $(man_pages)
