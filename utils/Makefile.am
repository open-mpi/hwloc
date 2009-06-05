# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

AM_CFLAGS += $(CAIRO_CFLAGS) $(XML_CFLAGS)
AM_LDFLAGS += $(CAIRO_LIBS) $(XML_LIBS)

EXTRA_DIST = lstopo.h

bin_PROGRAMS = lstopo topomask topobind

lstopo_SOURCES = lstopo.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c lstopo-xml.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += lstopo-windows.c
endif !HAVE_WINDOWS

LIBS = $(top_builddir)/src/libtopology.la
