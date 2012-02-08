# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/hwloc.am

AM_CFLAGS += $(XML_CFLAGS)

lib_LTLIBRARIES = libhwloc.la

EXTRA_DIST = dolib.c

libhwloc_la_SOURCES = topology.c traversal.c topology-synthetic.c bind.c cpuset.c
libhwloc_la_LDFLAGS = -version-number $(libhwloc_so_version) $(XML_LIBS)

if HAVE_GCC
libhwloc_la_LDFLAGS += -no-undefined
endif

if HAVE_WINDOWS
libhwloc_la_LDFLAGS += -Xlinker --output-def -Xlinker .libs/libhwloc.def

if HAVE_MS_LIB
.libs/libhwloc.lib: libhwloc.la dolib
	./dolib "$(HWLOC_MS_LIB)" X86 .libs/libhwloc.def libhwloc-$(HWLOC_SOVERSION) .libs/libhwloc.lib
all-local: .libs/libhwloc.lib
endif

install-exec-hook:
	$(INSTALL) .libs/libhwloc.def $(DESTDIR)$(libdir)
if HAVE_MS_LIB
	$(INSTALL) .libs/libhwloc.lib $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libhwloc.exp $(DESTDIR)$(libdir)
endif
endif

if HAVE_XML
libhwloc_la_SOURCES += topology-xml.c
else !HAVE_XML
EXTRA_DIST += topology-xml.c
endif !HAVE_XML

if HAVE_SOLARIS
libhwloc_la_SOURCES += topology-solaris.c
else !HAVE_SOLARIS
EXTRA_DIST += topology-solaris.c
endif !HAVE_SOLARIS

if HAVE_LINUX
libhwloc_la_SOURCES += topology-linux.c
else !HAVE_LINUX
EXTRA_DIST += topology-linux.c
endif !HAVE_LINUX

if HAVE_AIX
libhwloc_la_SOURCES += topology-aix.c
libhwloc_la_LDFLAGS += -lpthread
else !HAVE_AIX
EXTRA_DIST += topology-aix.c
endif !HAVE_AIX

if HAVE_OSF
libhwloc_la_SOURCES += topology-osf.c
libhwloc_la_LDFLAGS += -lnuma -lpthread
else !HAVE_OSF
EXTRA_DIST += topology-osf.c
endif !HAVE_OSF

if HAVE_HPUX
libhwloc_la_SOURCES += topology-hpux.c
libhwloc_la_LDFLAGS += -lpthread
else !HAVE_HPUX
EXTRA_DIST += topology-hpux.c
endif !HAVE_HPUX

if HAVE_WINDOWS
libhwloc_la_SOURCES += topology-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += topology-windows.c
endif !HAVE_WINDOWS

if HAVE_DARWIN
libhwloc_la_SOURCES += topology-darwin.c
else !HAVE_DARWIN
EXTRA_DIST += topology-darwin.c
endif !HAVE_DARWIN

xml_DATA = $(srcdir)/hwloc.dtd
xmldir = $(pkgdatadir)
EXTRA_DIST += hwloc.dtd
