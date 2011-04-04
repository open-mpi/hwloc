# Copyright © 2009-2011 INRIA.  All rights reserved.
# Copyright © 2009-2011 Université Bordeaux 1
# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

SUBDIRS = ports xml
DIST_SUBDIRS = ports xml linux

if HWLOC_HAVE_LINUX
SUBDIRS += linux
endif HWLOC_HAVE_LINUX

LDADD =

if HWLOC_BUILD_TESTS
check_PROGRAMS = hwloc_bitmap \
	hwloc_bitmap_string \
	hwloc_get_closest_objs \
	hwloc_get_obj_covering_cpuset \
	hwloc_get_cache_covering_cpuset \
	hwloc_get_largest_objs_inside_cpuset \
	hwloc_get_next_obj_covering_cpuset \
	hwloc_get_obj_inside_cpuset \
	hwloc_get_shared_cache_covering_obj \
	hwloc_get_obj_below_array_by_type \
	hwloc_bitmap_first_last_weight \
	hwloc_bitmap_singlify \
	hwloc_type_depth \
	hwloc_bind \
	hwloc_get_last_cpu_location \
	hwloc_object_userdata \
	hwloc_synthetic \
	hwloc_is_thissystem \
	hwloc_distances \
	hwloc_set_distances \
	hwloc_groups \
	hwloc_insert_misc \
	hwloc_topology_restrict

if HWLOC_HAVE_LINUX_LIBNUMA
check_PROGRAMS += linux-libnuma
endif HWLOC_HAVE_LINUX_LIBNUMA

if HWLOC_HAVE_SCHED_SETAFFINITY
check_PROGRAMS += glibc-sched
endif HWLOC_HAVE_SCHED_SETAFFINITY

if HWLOC_HAVE_LIBIBVERBS
check_PROGRAMS += openfabrics-verbs
endif HWLOC_HAVE_LIBIBVERBS

if HWLOC_HAVE_MYRIEXPRESS
check_PROGRAMS += myriexpress
endif HWLOC_HAVE_MYRIEXPRESS

if HWLOC_HAVE_CUDA
check_PROGRAMS += cuda
endif HWLOC_HAVE_CUDA

if HWLOC_HAVE_CUDART
check_PROGRAMS += cudart
endif HWLOC_HAVE_CUDART

if HWLOC_HAVE_XML
check_PROGRAMS += xmlbuffer
endif HWLOC_HAVE_XML

TESTS = $(check_PROGRAMS)

# The library has a different name depending on whether we are
# building in standalone or embedded mode.
if HWLOC_BUILD_STANDALONE
hwloc_lib = libhwloc.la
else
hwloc_lib = libhwloc_embedded.la
endif

LDADD += $(HWLOC_top_builddir)/src/$(hwloc_lib)

linux_libnuma_LDFLAGS = -lnuma
openfabrics_verbs_LDFLAGS = -libverbs
myriexpress_LDFLAGS = -lmyriexpress
cuda_LDFLAGS = -lcuda
cudart_LDFLAGS = -lcuda -lcudart
xmlbuffer_LDFLAGS = $(HWLOC_XML_LIBS)
if !HWLOC_HAVE_WINDOWS
hwloc_bind_LDFLAGS = -lpthread
endif !HWLOC_HAVE_WINDOWS

endif HWLOC_BUILD_TESTS

# ship the embedded test code but don't actually enter it automatically
EXTRA_DIST = 	embedded/autogen.sh \
		embedded/configure.ac \
		embedded/Makefile.am \
		embedded/do_test.c \
		embedded/main.c \
		embedded/run-embedded-tests.sh \
		embedded/README.txt \
		embedded/config/README.txt
