# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_GCC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# XXX: Add your XML input files here.
xml_inputs = \
	16amd64-8nd2c-cpusets.xml \
	16em64t-4s2c2t.xml \
	16em64t-4s2c2t-offlines.xml 

if HWLOC_HAVE_XML
TESTS = $(xml_inputs)
else !HWLOC_HAVE_XML
TESTS =
endif !HWLOC_HAVE_XML

EXTRA_DIST = $(xml_inputs)

TESTS_ENVIRONMENT = $(builddir)/test-topology.sh
