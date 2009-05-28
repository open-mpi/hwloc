# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SUBDIRS = linux ports

TESTS = topo_object_cpuset_snprintf+topo_cpuset_from_string \
	topo_find_closest_objects \
	topo_find_cpuset_objects \
	topo_find_cpuset_covering_object \
	topo_find_cpuset_covering_cache \
	topo_find_shared_cache_above \
	object_userdata \
	synthetic-topology

check_PROGRAMS = $(TESTS)

LIBS = $(top_builddir)/src/libtopology.la
