# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright 2009 Cisco Systems, Inc.  All rights reserved.

# Only install the headers if we're in standalone mode

if HWLOC_BUILD_STANDALONE
include_HEADERS = hwloc.h
include_hwlocdir = $(includedir)/hwloc
include_hwloc_HEADERS = \
        hwloc/cpuset.h \
        hwloc/helper.h \
        hwloc/openfabrics-verbs.h \
        hwloc/plpa.h \
        hwloc/rename.h
nodist_include_hwloc_HEADERS = hwloc/config.h 

noinst_HEADERS = \
        private/private.h \
        private/debug.h \
        private/cpuset.h

if HWLOC_HAVE_LINUX
include_hwloc_HEADERS += \
        hwloc/linux.h \
        hwloc/linux-libnuma.h
endif HWLOC_HAVE_LINUX

if HWLOC_HAVE_SCHED_SETAFFINITY
include_hwloc_HEADERS += hwloc/glibc-sched.h
endif HWLOC_HAVE_SCHED_SETAFFINITY

endif HWLOC_BUILD_STANDALONE
