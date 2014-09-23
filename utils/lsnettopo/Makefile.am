# Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CPPFLAGS = \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include

bin_PROGRAMS = \
        lsnettopo

lsnettopo_SOURCES = \
        lsnettopo.c

lsnettopo_LDADD = \
        $(top_builddir)/netloc/libnetloc.la
