# Copyright © 2009-2014 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

ACLOCAL_AMFLAGS = -I./config -I./netloc-tree/config

SUBDIRS = netloc-tree

AM_CPPFLAGS = $(HWLOC_EMBEDDED_CPPFLAGS) $(NETLOC_EMBEDDED_CPPFLAGS)

TESTS = main

noinst_PROGRAMS = main
noinst_LTLIBRARIES = libdo_test.la

libdo_test_la_SOURCES = do_test.c

main_SOURCES = main.c
main_LDADD = libdo_test.la $(HWLOC_EMBEDDED_LDADD) $(NETLOC_EMBEDDED_LDADD) $(HWLOC_EMBEDDED_LIBS) $(NETLOC_EMBEDDED_LIBS)
main_DEPENDENCIES = libdo_test.la

EXTRA_DIST = run-embedded-tests.sh
