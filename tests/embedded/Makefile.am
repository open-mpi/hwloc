# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

ACLOCAL_AMFLAGS = -I ./config

SUBDIRS = hwloc-tree

noinst_PROGRAMS = main

AM_CPPFLAGS = $(HWLOC_CPPFLAGS)

main_SOURCES = main.c
main_LDADD = hwloc-tree/src/libhwloc_embedded.la
