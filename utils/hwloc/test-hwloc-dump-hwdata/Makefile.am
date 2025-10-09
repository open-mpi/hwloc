# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2016 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

dumps = 

if HWLOC_HAVE_BUNZIPP
TESTS = $(dumps)
endif HWLOC_HAVE_BUNZIPP

EXTRA_DIST = $(dumps)

LOG_COMPILER = $(builddir)/test-hwloc-dump-hwdata.sh
