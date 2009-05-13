# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

# XXX: Add your sysfs tarballs here.  Each tarball must contain an
# `output' file showing the excepted output of `lstopo'.
sysfs_tarballs = \
		2amd64-2nd.tar.gz \
		2ps3-1d2t.tar.gz \
		4ia64-4d.tar.gz \
		4ppc-1d4c.tar.gz \
		8amd64-4nd2c.tar.gz  \
		8em64t-1nd4c2t.tar.gz \
		8ia64-2nd2d2d.tar.gz \
		8ia64-4d2c.tar.gz \
		16amd64-8nd2c.tar.gz \
		16em64t-4d2c2t.tar.gz \
		64fake-4n2d2$$2c2t.tar.gz

if HAVE_LINUX
TESTS = $(sysfs_tarballs)
else !HAVE_LINUX
TESTS =
endif !HAVE_LINUX

EXTRA_DIST = $(sysfs_tarballs)

TESTS_ENVIRONMENT = $(builddir)/test-topology.sh
