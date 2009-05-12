# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

AM_CFLAGS += $(CAIRO_CFLAGS)
AM_LDFLAGS += $(CAIRO_LIBS)

EXTRA_DIST =

bin_PROGRAMS = lstopo topomask

lstopo_SOURCES = lstopo.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c
if HAVE_WINDOWS
lstopo_SOURCES += lstopo-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += lstopo-windows.c
endif !HAVE_WINDOWS

topomask_SOURCES = topomask.c

LIBS = $(top_builddir)/src/libtopology.la
