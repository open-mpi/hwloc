# Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013-2014 University of Wisconsin-La Crosse.
#                         All rights reserved.
# Copyright © 2016 Inria.  All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CPPFLAGS = \
        $(JANSSON_CPPFLAGS) \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include \
        -DNETLOC_ABS_TOP_SRCDIR=\"$(abs_top_srcdir)/\"

check_PROGRAMS = \
        test_API \
        test_ETH_API \
        test_ETH_verbose \
        test_find_neighbors \
        test_metadata \
        test_conv \
        test_map \
        test_map_hwloc \
        hwloc_compress \
        map_paths \
        netloc_hello \
        netloc_nodes \
        netloc_all

LDADD = $(top_builddir)/netloc/libnetloc.la

if BUILD_NETLOCSCOTCH
check_PROGRAMS += scotch
scotch_LDADD = $(top_builddir)/netloc/libnetlocscotch.la -lscotch -lscotcherr -lslurm
endif BUILD_NETLOCSCOTCH

test_map_LDADD = $(LDADD) $(top_builddir)/hwloc/libhwloc.la
map_paths_LDADD = $(LDADD) $(top_builddir)/hwloc/libhwloc.la
