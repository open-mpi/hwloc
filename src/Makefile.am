# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

AM_CFLAGS = $(HWLOC_GCC_CFLAGS) $(HWLOC_XML_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

lib_LTLIBRARIES = libhwloc.la

EXTRA_DIST = dolib.c

libhwloc_la_SOURCES = topology.c traversal.c topology-synthetic.c bind.c cpuset.c
libhwloc_la_LDFLAGS = -version-number $(libhwloc_so_version) $(HWLOC_XML_LIBS)

if HWLOC_HAVE_GCC
libhwloc_la_LDFLAGS += -no-undefined
endif

if HWLOC_HAVE_WINDOWS
libhwloc_la_LDFLAGS += -Xlinker --output-def -Xlinker .libs/libhwloc.def

if HWLOC_HAVE_MS_LIB
.libs/libhwloc.lib: libhwloc.la dolib
	./dolib "$(HWLOC_MS_LIB)" X86 .libs/libhwloc.def libhwloc-$(HWLOC_SOVERSION) .libs/libhwloc.lib
all-local: .libs/libhwloc.lib
endif

install-exec-hook:
	$(INSTALL) .libs/libhwloc.def $(DESTDIR)$(libdir)
if HWLOC_HAVE_MS_LIB
	$(INSTALL) .libs/libhwloc.lib $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libhwloc.exp $(DESTDIR)$(libdir)
endif
endif

if HWLOC_HAVE_XML
libhwloc_la_SOURCES += topology-xml.c
else !HWLOC_HAVE_XML
EXTRA_DIST += topology-xml.c
endif !HWLOC_HAVE_XML

if HWLOC_HAVE_SOLARIS
libhwloc_la_SOURCES += topology-solaris.c
else !HWLOC_HAVE_SOLARIS
EXTRA_DIST += topology-solaris.c
endif !HWLOC_HAVE_SOLARIS

if HWLOC_HAVE_LINUX
libhwloc_la_SOURCES += topology-linux.c
else !HWLOC_HAVE_LINUX
EXTRA_DIST += topology-linux.c
endif !HWLOC_HAVE_LINUX

if HWLOC_HAVE_AIX
libhwloc_la_SOURCES += topology-aix.c
libhwloc_la_LDFLAGS += -lpthread
else !HWLOC_HAVE_AIX
EXTRA_DIST += topology-aix.c
endif !HWLOC_HAVE_AIX

if HWLOC_HAVE_OSF
libhwloc_la_SOURCES += topology-osf.c
libhwloc_la_LDFLAGS += -lnuma -lpthread
else !HWLOC_HAVE_OSF
EXTRA_DIST += topology-osf.c
endif !HWLOC_HAVE_OSF

if HWLOC_HAVE_HPUX
libhwloc_la_SOURCES += topology-hpux.c
libhwloc_la_LDFLAGS += -lpthread
else !HWLOC_HAVE_HPUX
EXTRA_DIST += topology-hpux.c
endif !HWLOC_HAVE_HPUX

if HWLOC_HAVE_WINDOWS
libhwloc_la_SOURCES += topology-windows.c
else !HWLOC_HAVE_WINDOWS
EXTRA_DIST += topology-windows.c
endif !HWLOC_HAVE_WINDOWS

if HWLOC_HAVE_DARWIN
libhwloc_la_SOURCES += topology-darwin.c
else !HWLOC_HAVE_DARWIN
EXTRA_DIST += topology-darwin.c
endif !HWLOC_HAVE_DARWIN

xml_DATA = $(srcdir)/hwloc.dtd
xmldir = $(pkgdatadir)
EXTRA_DIST += hwloc.dtd
