# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_GCC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

SRC=$(top_srcdir)/src

if HWLOC_HAVE_LINUX
check_LTLIBRARIES = libhwloc-ports.la
endif

libhwloc_ports_la_SOURCES = $(SRC)/topology.c $(SRC)/traversal.c $(SRC)/topology-synthetic.c
libhwloc_ports_la_CFLAGS = -I$(top_srcdir)/tests/ports/include -DHWLOC_COMPILE_PORTS

libhwloc_ports_la_SOURCES += $(SRC)/topology-solaris.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_SOLARIS_SYS -DHAVE_LIBLGRP -DHAVE_LIBKSTAT 
libhwloc_ports_la_SOURCES += $(SRC)/topology-aix.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_AIX_SYS
libhwloc_ports_la_SOURCES += $(SRC)/topology-osf.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_OSF_SYS
libhwloc_ports_la_SOURCES += $(SRC)/topology-windows.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_WIN_SYS
libhwloc_ports_la_SOURCES += $(SRC)/topology-darwin.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_DARWIN_SYS
libhwloc_ports_la_SOURCES += $(SRC)/topology-hpux.c
libhwloc_ports_la_CFLAGS  += -DHWLOC_HPUX_SYS

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

