# Copyright © 2015 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your expected output file here.
# By default, it shows the output of `lstopo --of xml -'.
cpuid_outputs = \
	Intel-Haswell-2xXeon-E5-2680v3.output

# Each output `xyz.output' must have a corresponding tarball `xyz.tar.bz2'
# specifying which tarball to use
cpuid_tarballs = \
	Intel-Haswell-2xXeon-E5-2680v3.tar.bz2

# Each output `xyz.output' may have a corresponding `xyz.options'
# file modifying the behavior of lstopo
cpuid_options = 

# Each output `xyz.output' may have a corresponding `xyz.env'
# modifying the environment of lstopo
cpuid_envs = 

if HWLOC_HAVE_BUNZIPP
TESTS = $(cpuid_outputs)
endif HWLOC_HAVE_BUNZIPP

EXTRA_DIST = $(cpuid_outputs) $(cpuid_tarballs) $(cpuid_options) $(cpuid_envs)

LOG_COMPILER = $(builddir)/test-topology.sh
