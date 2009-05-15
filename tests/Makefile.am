# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SUBDIRS = linux ports

TESTS = cpuset_string cpuset_object topo_find_closest_objects userdata

check_PROGRAMS = $(TESTS)

LIBS = $(top_builddir)/src/libtopology.la
