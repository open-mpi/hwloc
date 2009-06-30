# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SUBDIRS = linux ports

check_PROGRAMS = topo_object_cpuset_snprintf+topo_cpuset_from_string \
	topo_get_closest_objs \
	topo_get_cpuset_covering_obj \
	topo_get_cpuset_covering_cache \
	topo_get_cpuset_objs \
	topo_get_next_obj_above_cpuset \
	topo_get_obj_below_cpuset \
	topo_get_shared_cache_above \
	topo_cpuset_singlify \
	topo_type_depth \
	topo_bind \
	object_userdata \
	synthetic-topology

EXTRA_DIST = lstopo_xml.sh

if HAVE_LINUX_LIBNUMA
check_PROGRAMS += linux-libnuma
else !HAVE_LINUX_LIBNUMA
EXTRA_DIST += linux-libnuma
endif !HAVE_LINUX_LIBNUMA

if HAVE_SCHED_SETAFFINITY
check_PROGRAMS += glibc-sched
else !HAVE_SCHED_SETAFFINITY
EXTRA_DIST += glibc-sched
endif !HAVE_SCHED_SETAFFINITY

TESTS = $(check_PROGRAMS) \
	$(srcdir)/lstopo_xml.sh

LIBS = $(top_builddir)/src/libtopology.la

linux_libnuma_LDFLAGS = -lnuma
topo_bind_LDFLAGS = -lpthread
