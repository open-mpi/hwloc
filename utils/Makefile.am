# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

include $(top_srcdir)/hwloc.am

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

if HWLOC_INSTALL_UTILS_MAN
dist_man1_MANS = lstopo.1 hwloc-bind.1 hwloc-distrib.1 hwloc-mask.1
MAINTAINERCLEANFILES = $(dist_man1_MANS)

if HWLOC_HAVE_UTILS_MAN
else
  ifneq ($(HELP2MAN),)
HWLOC_HELP2MAN=$(HELP2MAN) -m "Hardware Locality commands" -S "INRIA" -N 

lstopo.1: lstopo
	$(HWLOC_HELP2MAN) -n "Show the topology of the system" ./$^ -o $@
hwloc-bind.1: hwloc-bind
	$(HWLOC_HELP2MAN) -n "Bind a command to given objects or cpu masks" ./$^ -o $@
hwloc-distrib.1: hwloc-distrib
	$(HWLOC_HELP2MAN) -n "Build a number of cpu masks distributed on the system" ./$^ -o $@
hwloc-mask.1: hwloc-mask
	$(HWLOC_HELP2MAN) -n "Build a cpu mask" ./$^ -o $@
  endif
endif
endif

install-exec-hook:
	$(RM) $(DESTDIR)$(bindir)/hwloc-ls $(DESTDIR)$(bindir)/hwloc-info
	$(LN_S) lstopo $(DESTDIR)$(bindir)/hwloc-ls
	$(LN_S) lstopo $(DESTDIR)$(bindir)/hwloc-info

uninstall-local:
	$(RM) $(DESTDIR)$(bindir)/hwloc-ls 
	$(RM) $(DESTDIR)$(bindir)/hwloc-info
