# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

AM_LDFLAGS += $(XML_LIBS)

SUBDIRS = linux ports

TESTS = topo_object_cpuset_snprintf+topo_cpuset_from_string \
	topo_get_closest_objs \
	topo_get_cpuset_covering_obj \
	topo_get_cpuset_covering_cache \
	topo_get_cpuset_objs \
	topo_get_next_obj_above_cpuset \
	topo_get_obj_below_cpuset \
	topo_get_shared_cache_above \
	topo_cpuset_singlify \
	object_userdata \
	synthetic-topology

check_PROGRAMS = $(TESTS)

LIBS = $(top_builddir)/src/libtopology.la
