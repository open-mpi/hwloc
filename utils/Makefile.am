# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

EXTRA_DIST = lstopo.h test-topodistrib.output

bin_PROGRAMS = lstopo topomask topobind topodistrib

lstopo_SOURCES = lstopo.c lstopo-color.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c lstopo-xml.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += lstopo-windows.c
endif !HAVE_WINDOWS
lstopo_CFLAGS = $(CAIRO_CFLAGS) $(XML_CFLAGS)
lstopo_LDFLAGS = $(CAIRO_LIBS) $(XML_LIBS) -lm

LIBS = $(top_builddir)/src/libtopology.la

if HAVE_MINGW32
else
TESTS = test-topodistrib.sh
endif

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
