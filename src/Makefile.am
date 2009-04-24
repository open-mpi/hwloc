# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

lib_LTLIBRARIES = libtopology.la

EXTRA_DIST =

libtopology_la_SOURCES = topology.c allocator.c traversal.c
if HAVE_LINUX
libtopology_la_SOURCES += topology-linux.c
else !HAVE_LINUX
EXTRA_DIST += topology-linux.c
endif !HAVE_LINUX
