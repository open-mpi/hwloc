# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

EXTRA_DIST = lstopo.h

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
