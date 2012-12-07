# Copyright © 2009-2010 inria.  All rights reserved.
# Copyright © 2009, 2011-2012 Université Bordeaux 1
# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

SRC = $(HWLOC_top_srcdir)/src

if HWLOC_HAVE_LINUX
check_LTLIBRARIES = \
	libhwloc-aix-port.la \
	libhwloc-darwin-port.la \
	libhwloc-freebsd-port.la \
	libhwloc-hpux-port.la \
	libhwloc-osf-port.la \
	libhwloc-solaris-port.la \
	libhwloc-windows-port.la
endif HWLOC_HAVE_LINUX

# Note that AC_CONFIG_LINKS sets up the sym links for the files in
# this directory (back to the $top_srcdir/src directory).  So if you
# need more sym-linked files in here, go edit configure.ac.  Note that
# we have to use sym links in here rather than just directly
# referencing the files via $HWLOC_top_srcdir/src/foo.c because of
# dependencies issues when using the Automake option "subdir-objects".
# We nodist these because they're created by configure.

common_CPPFLAGS = \
        $(HWLOC_CPPFLAGS) \
        -I$(HWLOC_top_srcdir)/include \
        -I$(HWLOC_top_srcdir)/tests/ports/include \
        -DHWLOC_COMPILE_PORTS \
        -DHWLOC_INSIDE_LIBHWLOC

nodist_libhwloc_aix_port_la_SOURCES = topology-aix.c
libhwloc_aix_port_la_SOURCES = \
	include/procinfo.h \
	include/sys/rset.h \
	include/sys/systemcfg.h \
	include/sys/thread.h
libhwloc_aix_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_AIX_SYS

nodist_libhwloc_darwin_port_la_SOURCES = topology-darwin.c
libhwloc_darwin_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_DARWIN_SYS

nodist_libhwloc_freebsd_port_la_SOURCES = topology-freebsd.c
libhwloc_freebsd_port_la_SOURCES = \
	include/pthread_np.h \
	include/sys/cpuset.h
libhwloc_freebsd_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_FREEBSD_SYS -DHAVE_SYS_CPUSET_H -DHAVE_CPUSET_SETID

nodist_libhwloc_hpux_port_la_SOURCES = topology-hpux.c
libhwloc_hpux_port_la_SOURCES = \
	include/sys/mpctl.h
libhwloc_hpux_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_HPUX_SYS

nodist_libhwloc_osf_port_la_SOURCES = topology-osf.c
libhwloc_osf_port_la_SOURCES = \
	include/cpuset.h \
	include/numa.h \
	include/radset.h
libhwloc_osf_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_OSF_SYS

nodist_libhwloc_solaris_port_la_SOURCES = topology-solaris.c topology-solaris-chiptype.c
libhwloc_solaris_port_la_SOURCES = include/kstat.h include/picl.h include/sys/lgrp_user.h include/sys/procset.h include/sys/systeminfo.h
libhwloc_solaris_port_la_CPPFLAGS = $(common_CPPFLAGS) \
	-DHWLOC_SOLARIS_SYS -DHAVE_LIBLGRP -DHAVE_LIBKSTAT -DHAVE_LGRP_LATENCY_COOKIE

nodist_libhwloc_windows_port_la_SOURCES = topology-windows.c
libhwloc_windows_port_la_SOURCES = include/windows.h
libhwloc_windows_port_la_CPPFLAGS = $(common_CPPFLAGS) \
        -DHWLOC_WIN_SYS
	
#        include/sys/sysctl.h
#        include/sys/processor.h
