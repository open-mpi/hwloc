# Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
# Copyright © 2015-2016 Inria.  All rights reserved.
#
# Copyright © 2016 Inria.  All rights reserved.
# See COPYING in top-level directory.
#
# $HEADER$
#
AM_CPPFLAGS = \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include

bin_PROGRAMS = \
        netloc_ib_extract_dats

netloc_ib_extract_dats_SOURCES = \
        netloc_ib_extract_dats.c

netloc_ib_extract_dats_LDADD = \
        $(top_builddir)/netloc/libnetloc.la
