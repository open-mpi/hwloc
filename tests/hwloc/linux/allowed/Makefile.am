# Copyright © 2009-2023 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

allowed_tests = \
	test1.test

allowed_outputs = \
	test1.xml

allowed_fsroots = \
	test1.fsroot.tar.bz2

if HWLOC_HAVE_OPENAT
if HWLOC_HAVE_BUNZIPP
TESTS = $(allowed_tests)
endif HWLOC_HAVE_BUNZIPP
endif HWLOC_HAVE_OPENAT

EXTRA_DIST = $(allowed_tests) $(allowed_outputs) $(allowed_fsroots)

# .test is in TEXT_EXTENSIONS by default, hence it uses its own test configuration
# variables prefixed with TEST_
TEST_LOG_COMPILER = $(abs_builddir)/test-topology.sh

check-local:
	if test -x extra/extra-tests.sh; then extra/extra-tests.sh "$(TEST_LOG_COMPILER)"; fi
