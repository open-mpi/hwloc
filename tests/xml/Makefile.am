# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/hwloc.am

# XXX: Add your XML input files here.
xml_inputs = 16em64t-4s2c2t.xml

if HAVE_XML
TESTS = $(xml_inputs)
else !HAVE_XML
TESTS =
endif !HAVE_XML

EXTRA_DIST = $(xml_inputs)

TESTS_ENVIRONMENT = $(builddir)/test-topology.sh
