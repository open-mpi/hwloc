# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/hwloc.am

EXTRA_DIST = lstopo.h topomask.h test-topodistrib.output

bin_PROGRAMS = lstopo topomask topobind topodistrib

lstopo_SOURCES = lstopo.c lstopo-color.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c lstopo-xml.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += lstopo-windows.c
endif !HAVE_WINDOWS
lstopo_CFLAGS = $(CAIRO_CFLAGS) $(XML_CFLAGS)
lstopo_LDFLAGS = $(CAIRO_LIBS) $(XML_LIBS) -lm

LIBS = $(top_builddir)/src/libhwloc.la

if HAVE_MINGW32
else
TESTS = test-topodistrib.sh
endif

if HWLOC_INSTALL_UTILS_MAN
dist_man1_MANS = lstopo.1 topobind.1 topodistrib.1 topomask.1
MAINTAINERCLEANFILES = $(dist_man1_MANS)

if HWLOC_HAVE_UTILS_MAN
else
  ifneq ($(HELP2MAN),)
HWLOC_HELP2MAN=$(HELP2MAN) -m "Topology commands" -S "INRIA" -N 

lstopo.1: lstopo
	$(HWLOC_HELP2MAN) -n "Show the topology of the system" ./$^ -o $@
topobind.1: topobind
	$(HWLOC_HELP2MAN) -n "Bind a command to given objects or cpu masks" ./$^ -o $@
topodistrib.1: topodistrib
	$(HWLOC_HELP2MAN) -n "Build a number of cpu masks distributed on the system" ./$^ -o $@
topomask.1: topomask
	$(HWLOC_HELP2MAN) -n "Build a cpu mask" ./$^ -o $@
  endif
endif
endif
