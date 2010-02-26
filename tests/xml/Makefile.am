# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_GCC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# XXX: Add your XML input files here.
xml_inputs = \
	16amd64-8nd2c-cpusets.xml \
	16em64t-4s2c2t.xml \
	16em64t-4s2c2t-offlines.xml \
	8em64t-2mi2ma2c.xml

# Only run the tests if we're building standalone, because the tests
# call hwloc executables.
if HWLOC_BUILD_STANDALONE
if HWLOC_HAVE_XML
TESTS = $(xml_inputs)
endif HWLOC_HAVE_XML
endif HWLOC_BUILD_STANDALONE

EXTRA_DIST = $(xml_inputs)

TESTS_ENVIRONMENT = $(HWLOC_top_builddir)/tests/xml/test-topology.sh
