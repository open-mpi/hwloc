# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2009-2023 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

allowed_tests = \
	test1.test \
	cgroup2-ancestors.test \
	cgroup2-long-cpuset.test \
	cgroup2-long-cgroup.test \
	cgroup2-long-ancestors.test \
	cgroup2-long-mntpnt.test \
	cgroup1-long-cgroup.test \
	cpuset-long-cpuset.test

allowed_outputs = \
	test1.xml \
	cgroup2-ancestors.xml \
	cgroup2-long-cpuset.xml \
	cgroup2-long-cgroup.xml \
	cgroup2-long-ancestors.xml \
	cgroup2-long-mntpnt.xml \
	cgroup1-long-cgroup.xml \
	cpuset-long-cpuset.xml

allowed_fsroots = \
	test1.fsroot.tar.bz2 \
	cgroup2-ancestors.fsroot.tar.bz2 \
	cgroup2-long-cpuset.fsroot.tar.bz2 \
	cgroup2-long-cgroup.fsroot.tar.bz2 \
	cgroup2-long-ancestors.fsroot.tar.bz2 \
	cgroup2-long-mntpnt.fsroot.tar.bz2 \
	cgroup1-long-cgroup.fsroot.tar.bz2 \
	cpuset-long-cpuset.fsroot.tar.bz2

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
