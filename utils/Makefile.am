# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

AM_CFLAGS += $(CAIRO_CFLAGS)
AM_LDFLAGS += $(CAIRO_LIBS)

bin_PROGRAMS = lstopo closest

lstopo_SOURCES = lstopo.c lstopo-text.c lstopo-draw.c lstopo-fig.c lstopo-cairo.c
closest_SOURCES = closest.c

LIBS = $(top_builddir)/src/libtopology.la
