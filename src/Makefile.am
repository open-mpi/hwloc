# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

lib_LTLIBRARIES = libtopology.la

EXTRA_DIST =

libtopology_la_SOURCES = topology.c topology-solaris.c allocator.c traversal.c
libtopology_la_LDFLAGS = -no-undefined
if HAVE_LINUX
libtopology_la_SOURCES += topology-linux.c
else !HAVE_LINUX
EXTRA_DIST += topology-linux.c
endif !HAVE_LINUX

if HAVE_AIX
libtopology_la_SOURCES += topology-aix.c
else !HAVE_AIX
EXTRA_DIST += topology-aix.c
endif !HAVE_AIX

if HAVE_OSF
libtopology_la_SOURCES += topology-osf.c
else !HAVE_OSF
EXTRA_DIST += topology-osf.c
endif !HAVE_OSF

if HAVE_WINDOWS
libtopology_la_SOURCES += topology-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += topology-windows.c
endif !HAVE_WINDOWS
