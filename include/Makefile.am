# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright 2009 Cisco Systems, Inc.  All rights reserved.

include $(top_srcdir)/hwloc.am

include_HEADERS = hwloc.h
include_hwlocdir = $(includedir)/hwloc
include_hwloc_HEADERS = hwloc/cpuset.h hwloc/helper.h hwloc/openfabrics-verbs.h
nodist_include_hwloc_HEADERS = hwloc/config.h 

noinst_HEADERS = private/private.h private/debug.h private/cpuset.h

if HAVE_LINUX
include_hwloc_HEADERS += hwloc/linux.h hwloc/linux-libnuma.h
else !HAVE_LINUX
noinst_HEADERS += hwloc/linux.h hwloc/linux-libnuma.h
endif !HAVE_LINUX

if HAVE_SCHED_SETAFFINITY
include_hwloc_HEADERS += hwloc/glibc-sched.h
else !HAVE_SCHED_SETAFFINITY
noinst_HEADERS += hwloc/glibc-sched.h
endif !HAVE_SCHED_SETAFFINITY
