# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

include_HEADERS = topology.h
include_topologydir = $(includedir)/topology
include_topology_HEADERS = topology/config.h topology/cpuset.h topology/cpuset-bits.h topology/helper.h

noinst_HEADERS = private/private.h private/debug.h

if HAVE_LINUX
include_topology_HEADERS += topology/linux-libnuma.h
else !HAVE_LINUX
noinst_HEADERS += topology/linux-libnuma.h
endif !HAVE_LINUX

if HAVE_SCHED_SETAFFINITY
include_topology_HEADERS += topology/glibc-sched.h
else !HAVE_SCHED_SETAFFINITY
noinst_HEADERS += topology/glibc-sched.h
endif !HAVE_SCHED_SETAFFINITY
