# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SRC=$(top_srcdir)/src

check_LTLIBRARIES = libtopology-ports.la

libtopology_ports_la_SOURCES = $(SRC)/topology.c $(SRC)/allocator.c $(SRC)/traversal.c $(SRC)/topology-synthetic.c
libtopology_ports_la_CFLAGS = -I$(top_srcdir)/tests/ports/include -DTOPO_COMPILE_PORTS -DHAVE_LIBLGRP -DHAVE_LIBKSTAT

libtopology_ports_la_SOURCES += $(SRC)/topology-solaris.c
libtopology_ports_la_SOURCES += $(SRC)/topology-aix.c
libtopology_ports_la_SOURCES += $(SRC)/topology-osf.c
libtopology_ports_la_SOURCES += $(SRC)/topology-windows.c
