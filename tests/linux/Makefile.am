# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your expected output file here.
# By default, it shows the output of `lstopo - -v'.
sysfs_outputs = \
		2amd64-2n.output \
		2ps3-2t.output \
		4ia64-4s.output \
		4ppc-4c.output \
		256ppc-8n8s4t.output \
		256ppc-8n8s4t-nocache.output \
		256ppc-8n8s4t-nosys.output \
		8amd64-4n2c.output  \
		8em64t-4c2t.output \
		8em64t-2s2ca2c.output \
		8em64t-2s4c-heterogeneous.output \
		8ia64-2n2s2d.output \
		8ia64-4s2c.output \
		16amd64-8n2c.output \
		16amd64-8n2c-cpusets.output \
		16amd64-8n2c-cpusets.xml.output \
		16amd64-8n2c-cpusets_noadmin.output
		16em64t-4s2c2t.output \
		16em64t-4s2c2t-offlines.output \
		16em64t-4s2c2t-offlines.xml.output \
		16em64t-4s2c2t-offlines_noadmin.output \
		16em64t-4s2c2t_ncaches.output \
		16em64t-4s2c2t_merge.output \
		16em64t-4s2c2t.xml.output \
		16em64t-2m4c2t.output \
		16ia64-8n2s.output \
		48amd64-4d2n6c-sparse.output \
		64fake-4n2s2ca2c2t.output \
		96em64t-4n4d3ca2co.output \
		128ia64-17n4s2c.output \
		256ia64-64n2s2c.output \
		4qs22-2s2t.output \
		2s390-2c.output \
		1alpha.output

# Each output `xyz.output' must have a corresponding tarball `xyz.tar.gz'
# or a corresponding `xyz.source' specifying which tarball to use
sysfs_tarballs = \
		2amd64-2n.tar.gz \
		2ps3-2t.tar.gz \
		4ia64-4s.tar.gz \
		4ppc-4c.tar.gz \
		256ppc-8n8s4t.tar.gz \
		256ppc-8n8s4t-nocache.source \
		256ppc-8n8s4t-nosys.source \
		8amd64-4n2c.tar.gz  \
		8em64t-4c2t.tar.gz \
		8em64t-2s2ca2c.tar.gz \
		8em64t-2s4c-heterogeneous.tar.gz \
		8ia64-2n2s2d.tar.gz \
		8ia64-4s2c.tar.gz \
		16amd64-8n2c.tar.gz \
		16amd64-8n2c-cpusets.tar.gz \
		16amd64-8n2c-cpusets.xml.source \
		16amd64-8n2c-cpusets_noadmin.source \
		16em64t-4s2c2t.tar.gz \
		16em64t-4s2c2t-offlines.tar.gz \
		16em64t-4s2c2t-offlines.xml.source \
		16em64t-4s2c2t-offlines_noadmin.source \
		16em64t-2m4c2t.tar.gz \
		16em64t-4s2c2t_merge.source \
		16em64t-4s2c2t_ncaches.source \
		16em64t-4s2c2t.xml.source \
		16ia64-8n2s.tar.gz \
		48amd64-4d2n6c-sparse.tar.gz \
		64fake-4n2s2ca2c2t.tar.gz \
		96em64t-4n4d3ca2co.tar.gz \
		128ia64-17n4s2c.tar.gz \
		256ia64-64n2s2c.tar.gz \
		4qs22-2s2t.tar.gz \
		2s390-2c.tar.gz \
		1alpha.tar.gz

# Each `xyz.output' may have a corresponding exclude `xyz.exclude'
sysfs_excludes = \
		256ppc-8n8s4t-nocache.exclude \
		256ppc-8n8s4t-nosys.exclude

# Each output `xyz.output' may have a corresponding `xyz.options'
# file modifying the behavior of lstopo
sysfs_options = \
		16amd64-8n2c-cpusets.xml.options \
		16amd64-8n2c-cpusets_noadmin.options \
		16em64t-4s2c2t-offlines.xml.options \
		16em64t-4s2c2t-offlines_noadmin.options \
		16em64t-4s2c2t_ncaches.options \
		16em64t-4s2c2t_merge.options \
		16em64t-4s2c2t.xml.options

if HWLOC_BUILD_TESTS
if HWLOC_HAVE_LINUX
if HWLOC_HAVE_OPENAT
TESTS = $(sysfs_outputs)
endif HWLOC_HAVE_OPENAT
endif HWLOC_HAVE_LINUX
endif HWLOC_BUILD_TESTS

EXTRA_DIST = $(sysfs_outputs) $(sysfs_tarballs) $(sysfs_excludes) $(sysfs_options)

TESTS_ENVIRONMENT = $(builddir)/test-topology.sh
