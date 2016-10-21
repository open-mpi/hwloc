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

EXTRA_DIST = \
        data/tests_extract.txt \
        data/tests_draw.txt \
        data/avakas.txz \
        data/plafrim.txz \
        data/plafrim2.txz \
        data/tests_scotch.txt \
        data/scotch/test_scotch_get_arch.sh \
        data/scotch/test_mpi_rank_file.sh \
        data/scotch/app.mat \
        data/scotch/resources.txt \
        data/scotch/arch.scotch \
        data/scotch/subarch.scotch

if FOUND_XZ
TESTS = \
        data/tests_extract.txt \
        data/tests_draw.txt

if BUILD_NETLOCSCOTCH
check_PROGRAMS = data/scotch/scotch_get_arch
TESTS += data/tests_scotch.txt
endif
endif

TEST_EXTENSIONS = .txt
TXT_LOG_COMPILER = ./tests.sh
