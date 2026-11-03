# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2009-2017 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

tests_outputs = \
	test1.output \
	cgroup2-ancestors.output \
	cgroup2-long-cpuset.output \
	cgroup2-long-cgroup.output \
	cgroup2-long-ancestors.output \
	cgroup2-long-mntpnt.output \
	cgroup1-long-cgroup.output \
	cpuset-long-cpuset.output

tests_synthetics = \
	test1.synthetic \
	cgroup2-ancestors.synthetic \
	cgroup2-long-cpuset.synthetic \
	cgroup2-long-cgroup.synthetic \
	cgroup2-long-ancestors.synthetic \
	cgroup2-long-mntpnt.synthetic \
	cgroup1-long-cgroup.synthetic \
	cpuset-long-cpuset.synthetic

tests_fsroots = \
	test1.fsroot.tar.bz2 \
	cgroup2-ancestors.fsroot.tar.bz2 \
	cgroup2-long-cpuset.fsroot.tar.bz2 \
	cgroup2-long-cgroup.fsroot.tar.bz2 \
	cgroup2-long-ancestors.fsroot.tar.bz2 \
	cgroup2-long-mntpnt.fsroot.tar.bz2 \
	cgroup1-long-cgroup.fsroot.tar.bz2 \
	cpuset-long-cpuset.fsroot.tar.bz2

tests_options = \
	test1.options \
	cgroup2-ancestors.options \
	cgroup2-long-cpuset.options \
	cgroup2-long-cgroup.options \
	cgroup2-long-ancestors.options \
	cgroup2-long-mntpnt.options \
	cgroup1-long-cgroup.options \
	cpuset-long-cpuset.options

tests_envs =

if HWLOC_HAVE_OPENAT
if HWLOC_HAVE_BUNZIPP
TESTS = $(tests_outputs)
endif HWLOC_HAVE_BUNZIPP
endif HWLOC_HAVE_OPENAT

EXTRA_DIST = $(tests_outputs) $(tests_synthetics) $(tests_fsroots) $(tests_options) $(tests_envs)

LOG_COMPILER = $(builddir)/test-topology.sh
