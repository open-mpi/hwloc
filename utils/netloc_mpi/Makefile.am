# Copyright © 2016 Inria.  All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CPPFLAGS = \
        $(JANSSON_CPPFLAGS) \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include

bin_PROGRAMS = \
        netloc_rank_order

if BUILD_NETLOCSCOTCH
bin_PROGRAMS += \
        netloc_mpi_rank_file
endif

netloc_rank_order_SOURCES = \
        netloc_rank_order.c

netloc_rank_order_LDADD = \
        $(top_builddir)/netloc/libnetloc.la

if BUILD_NETLOCSCOTCH
netloc_mpi_rank_file_SOURCES = \
        netloc_mpi_rank_file.c

netloc_mpi_rank_file_LDADD = \
        $(top_builddir)/netloc/libnetloc.la \
        $(top_builddir)/netloc/libnetlocscotch.la \
        -lscotch -lscotcherr
endif
