# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright 2009 Cisco Systems, Inc.  All rights reserved.

include $(top_srcdir)/hwloc.am

SUBDIRS = linux ports

check_PROGRAMS = hwloc_cpuset_string \
	hwloc_get_closest_objs \
	hwloc_get_obj_covering_cpuset \
	hwloc_get_cache_covering_cpuset \
	hwloc_get_largest_objs_inside_cpuset \
	hwloc_get_next_obj_covering_cpuset \
	hwloc_get_obj_inside_cpuset \
	hwloc_get_shared_cache_covering_obj \
	hwloc_cpuset_first_last_weight \
	hwloc_cpuset_singlify \
	hwloc_type_depth \
	hwloc_bind \
	hwloc_object_userdata \
	hwloc_synthetic \
	hwloc_is_thissystem

EXTRA_DIST = 

if HAVE_LINUX_LIBNUMA
check_PROGRAMS += linux-libnuma
else !HAVE_LINUX_LIBNUMA
EXTRA_DIST += linux-libnuma.c
endif !HAVE_LINUX_LIBNUMA

if HAVE_SCHED_SETAFFINITY
check_PROGRAMS += glibc-sched
else !HAVE_SCHED_SETAFFINITY
EXTRA_DIST += glibc-sched.c
endif !HAVE_SCHED_SETAFFINITY

if HAVE_LIBIBVERBS
check_PROGRAMS += openfabrics-verbs
else !HAVE_LIBIBVERBS
EXTRA_DIST += openfabrics-verbs.c
endif !HAVE_LIBIBVERBS

TESTS = $(check_PROGRAMS)

if HAVE_XML
TESTS += lstopo_xml.sh
endif

LIBS = $(top_builddir)/src/libhwloc.la

linux_libnuma_LDFLAGS = -lnuma
openfabrics_verbs_LDFLAGS = -libverbs
if HAVE_WINDOWS
else
hwloc_bind_LDFLAGS = -lpthread
endif
