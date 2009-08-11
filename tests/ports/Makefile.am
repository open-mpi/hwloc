# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

SRC=$(top_srcdir)/src

if HAVE_LINUX
check_LTLIBRARIES = libtopology-ports.la
endif

libtopology_ports_la_SOURCES = $(SRC)/topology.c $(SRC)/traversal.c $(SRC)/topology-synthetic.c
libtopology_ports_la_CFLAGS = -I$(top_srcdir)/tests/ports/include -DTOPO_COMPILE_PORTS -DHAVE_LIBLGRP -DHAVE_LIBKSTAT

libtopology_ports_la_SOURCES += $(SRC)/topology-solaris.c
libtopology_ports_la_SOURCES += $(SRC)/topology-aix.c
libtopology_ports_la_SOURCES += $(SRC)/topology-osf.c
libtopology_ports_la_SOURCES += $(SRC)/topology-windows.c
libtopology_ports_la_SOURCES += $(SRC)/topology-darwin.c
libtopology_ports_la_SOURCES += $(SRC)/topology-hpux.c

EXTRA_DIST = \
		include/windows.h \
		include/kstat.h \
		include/sys/rset.h \
		include/sys/lgrp_user.h \
		include/sys/sysctl.h \
		include/sys/procset.h \
		include/sys/processor.h \
		include/sys/thread.h \
		include/sys/mpctl.h \
		include/radset.h \
		include/cpuset.h \
		include/numa.h

