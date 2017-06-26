# Copyright © 2016-2017 Inria.  All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CPPFLAGS = $(HWLOC_CPPFLAGS) \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include
AM_LDFLAGS = $(HWLOC_LDFLAGS)

LDADD = $(top_builddir)/netloc/libnetloc.la \
        $(top_builddir)/netloc/libnetlocscotch.la \
        -lscotch -lscotcherr -lm -lpthread

if BUILD_NETLOCSCOTCH
bin_PROGRAMS = \
        netlocscotch_get_arch
endif

netlocscotch_get_arch_SOURCES = \
        netlocscotch_get_arch.c
