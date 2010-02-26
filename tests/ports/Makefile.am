# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_GCC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

SRC = $(HWLOC_top_srcdir)/src

if HWLOC_HAVE_LINUX
check_LTLIBRARIES = libhwloc-ports.la
endif

# Because embedded projects may use the Automake "subdir-object"
# option, we have to sym link other hwloc sources here rather than
# just using the sources elsewhere in the tree.
SYMLINKED_SOURCES = \
        topology.c \
        traversal.c \
        topology-synthetic.c \
        topology-solaris.c \
        topology-aix.c \
        topology-osf.c \
        topology-windows.c \
        topology-darwin.c \
        topology-freebsd.c \
        topology-hpux.c

$(SYMLINKED_SOURCES):
	if test ! -r $@ ; then \
	    $(LN_S) $(SRC)/$@ $(HWLOC_top_srcdir)/tests/ports/$@; \
	fi

clean-local:
	for file in $(SYMLINKED_SOURCES); do \
	    if -f $(HWLOC_top_srcdir)/tests/ports/$$file; then \
	        rm -f $(HWLOC_top_srcdir)/tests/ports/$$file; \
	    fi; \
	done

nodist_libhwloc_ports_la_SOURCES = $(SYMLINKED_SOURCES)

libhwloc_ports_la_SOURCES = \
        include/windows.h \
        include/kstat.h \
        include/sys/rset.h \
        include/sys/lgrp_user.h \
        include/sys/sysctl.h \
        include/sys/procset.h \
        include/sys/processor.h \
        include/sys/thread.h \
        include/sys/mpctl.h \
        include/sys/cpuset.h \
        include/radset.h \
        include/cpuset.h \
        include/numa.h \
        include/pthread_np.h

libhwloc_ports_la_CPPFLAGS = \
        -I$(HWLOC_top_srcdir)/include \
        -I$(HWLOC_top_srcdir)/tests/ports/include \
        -DHWLOC_COMPILE_PORTS \
        -DHWLOC_SOLARIS_SYS -DHAVE_LIBLGRP -DHAVE_LIBKSTAT \
        -DHWLOC_AIX_SYS \
        -DHWLOC_OSF_SYS \
        -DHWLOC_WIN_SYS \
        -DHWLOC_DARWIN_SYS \
        -DHWLOC_FREEBSD_SYS -DHAVE_SYS_CPUSET_H \
        -DHWLOC_HPUX_SYS

