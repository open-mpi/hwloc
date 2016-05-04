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

netloc_rank_order_SOURCES = \
        netloc_rank_order.c

netloc_rank_order_LDADD = \
        $(top_builddir)/netloc/libnetloc.la
