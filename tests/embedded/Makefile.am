# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.

ACLOCAL_AMFLAGS = -I./config -I./hwloc-tree/config

SUBDIRS = hwloc-tree

noinst_PROGRAMS = main

AM_CPPFLAGS = $(HWLOC_EMBEDDED_CPPFLAGS)

main_SOURCES = main.c
main_LDADD = $(HWLOC_EMBEDDED_LDADD)

EXTRA_DIST = run-embedded-tests.sh
