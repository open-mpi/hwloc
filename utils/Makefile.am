# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

EXTRA_DIST = lstopo.h test-topodistrib.sh test-topodistrib.output

bin_PROGRAMS = lstopo lstopo-nox topomask topobind topodistrib

lstopo_SOURCES = lstopo.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c lstopo-xml.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += lstopo-windows.c
endif !HAVE_WINDOWS

lstopo_nox_SOURCES = $(lstopo_SOURCES)

AM_CFLAGS += $(XML_CFLAGS)
AM_LDFLAGS += $(XML_LIBS)

lstopo_CPPFLAGS = $(AM_CPPFLAGS) $(CAIRO_CFLAGS)
lstopo_LDFLAGS = $(AM_LDFLAGS) $(CAIRO_LIBS)
lstopo_nox_CPPFLAGS = $(AM_CPPFLAGS) -DTOPO_DISABLE_GRAPHIC_LIBS

LIBS = $(top_builddir)/src/libtopology.la

TESTS = $(srcdir)/test-topodistrib.sh

if TOPO_INSTALL_UTILS_MAN
dist_man1_MANS = lstopo.1 topobind.1 topodistrib.1 topomask.1
MAINTAINERCLEANFILES = $(dist_man1_MANS)

if TOPO_HAVE_UTILS_MAN
else
  ifneq ($(HELP2MAN),)
TOPO_HELP2MAN=$(HELP2MAN) -m "Topology commands" -S "INRIA" -N --version-string @VERSION@

lstopo.1: lstopo
	$(TOPO_HELP2MAN) -n "Show the topology of the system" ./$^ -o $@
topobind.1: topobind
	$(TOPO_HELP2MAN) -n "Bind a command to a given cpu mask" ./$^ -o $@
topodistrib.1: topodistrib
	$(TOPO_HELP2MAN) -n "Build a number of cpu masks distributed on the system" ./$^ -o $@
topomask.1: topomask
	$(TOPO_HELP2MAN) -n "Build a cpu mask" ./$^ -o $@
  endif
endif
endif
