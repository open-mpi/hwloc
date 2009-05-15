# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SUBDIRS = linux ports

TESTS = cpuset_string topo_find_closest_objects topo_find_cpuset_ancestor_object object_userdata

check_PROGRAMS = $(TESTS)

LIBS = $(top_builddir)/src/libtopology.la
