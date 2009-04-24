# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

# XXX: Add your sysfs tarballs here.  Each tarball must contain an
# `output' file showing the excepted output of `lstopo'.
sysfs_tarballs =

if HAVE_LINUX
TESTS = $(sysfs_tarballs)
else !HAVE_LINUX
TESTS =
endif !HAVE_LINUX

EXTRA_DIST = $(sysfs_tarballs)

TESTS_ENVIRONMENT = $(srcdir)/test-topology.sh
