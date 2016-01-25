# Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013-2014 University of Wisconsin-La Crosse.
#                         All rights reserved.
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
        netloc_draw_to_json

netloc_draw_to_json_SOURCES = \
        netloc_draw_to_json.c

netloc_draw_to_json_LDADD = \
        $(top_builddir)/netloc/libnetloc.la
