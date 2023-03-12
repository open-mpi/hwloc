# Copyright © 2019-2023 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your tests here.
cpuid_sysfs_tests = \
	64amd64-4p2n4ca2co.test \
	64amd64-4p2n4ca2co+topoextnuma.test \
	5intel64-hybrid-lakefield.test

# Add your expected outputs here.
cpuid_sysfs_outputs = \
	64amd64-4p2n4ca2co.xml \
	5intel64-hybrid-lakefield.console

# Each output `xyz.output' must have a corresponding tarball `xyz.tar.bz2'
# or a corresponding `xyz.source' specifying which tarball to use
cpuid_sysfs_tarballs = \
	64amd64-4p2n4ca2co.tar.bz2 \
	5intel64-hybrid-lakefield.tar.bz2

if HWLOC_HAVE_OPENAT
if HWLOC_HAVE_BUNZIPP
TESTS = $(cpuid_sysfs_tests)
endif HWLOC_HAVE_BUNZIPP
endif HWLOC_HAVE_OPENAT

EXTRA_DIST = $(cpuid_sysfs_tests) $(cpuid_sysfs_outputs) $(cpuid_sysfs_tarballs)

# .test is in TEXT_EXTENSIONS by default, hence it uses its own test configuration
# variables prefixed with TEST_
TEST_LOG_COMPILER = $(abs_builddir)/test-topology.sh

check-local:
	if test -x extra/extra-tests.sh; then extra/extra-tests.sh "$(TEST_LOG_COMPILER)"; fi
