# Copyright © 2016 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

dumps = \
	knl-snc4h50.tar.bz2

if HWLOC_HAVE_BUNZIPP
TESTS = $(dumps)
endif HWLOC_HAVE_BUNZIPP

EXTRA_DIST = $(dumps)

LOG_COMPILER = $(builddir)/test-hwloc-dump-hwdata.sh


# Non-distributed tests in extra/*.output
HWLOC_EXTRA_CHECK_DIR = utils/hwloc/test-hwloc-dump-hwdata/extra
HWLOC_EXTRA_CHECK_TARGET = *.tar.bz2
include ../../../config/hwloc_extra.am
check-local: hwloc-extra-check
clean-local: hwloc-extra-check-clean

