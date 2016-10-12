# Copyright © 2016 Inria.  All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CFLAGS = $(NETLOC_CFLAGS)
AM_CPPFLAGS = $(NETLOC_CPPFLAGS)
AM_LDFLAGS = $(NETLOC_LDFLAGS)

LDADD = $(top_builddir)/netloc/libnetloc.la \
        $(top_builddir)/netloc/libnetlocscotch.la \
        -lscotch -lscotcherr

if BUILD_NETLOCSCOTCH
check_PROGRAMS = data/scotch/scotch_get_arch
endif

TESTS = \
        data/tests_extract.txt \
        data/tests_draw.txt \
        data/tests_scotch.txt
TEST_EXTENSIONS = .txt
TXT_LOG_COMPILER = ./tests.sh
