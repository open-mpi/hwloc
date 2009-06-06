# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am

AM_CFLAGS += $(XML_CFLAGS)

lib_LTLIBRARIES = libtopology.la

EXTRA_DIST =

libtopology_la_SOURCES = topology.c allocator.c traversal.c topology-synthetic.c topology-xml.c bind.c
libtopology_la_LDFLAGS = -version-number $(TOPO_SOVERSION)

if HAVE_GCC
libtopology_la_LDFLAGS += -no-undefined
endif

if HAVE_WINDOWS
libtopology_la_LDFLAGS += -Xlinker --output-def -Xlinker .libs/libtopology.def

if HAVE_MS_LIB
all-local: libtopology.la
	"$(TOPO_MS_LIB)" /machine:X86 /def:.libs/libtopology.def /name:libtopology-$(TOPO_SOVERSION) /out:.libs/libtopology.lib
endif

install-exec-hook:
	$(INSTALL) .libs/libtopology.def $(DESTDIR)$(libdir)
if HAVE_MS_LIB
	$(INSTALL) .libs/libtopology.lib $(DESTDIR)$(libdir)
	$(INSTALL) .libs/libtopology.exp $(DESTDIR)$(libdir)
endif
endif

if HAVE_SOLARIS
libtopology_la_SOURCES += topology-solaris.c
else !HAVE_SOLARIS
EXTRA_DIST += topology-solaris.c
endif !HAVE_SOLARIS

if HAVE_LINUX
libtopology_la_SOURCES += topology-linux.c
else !HAVE_LINUX
EXTRA_DIST += topology-linux.c
endif !HAVE_LINUX

if HAVE_AIX
libtopology_la_SOURCES += topology-aix.c
else !HAVE_AIX
EXTRA_DIST += topology-aix.c
endif !HAVE_AIX

if HAVE_OSF
libtopology_la_SOURCES += topology-osf.c
else !HAVE_OSF
EXTRA_DIST += topology-osf.c
endif !HAVE_OSF

if HAVE_WINDOWS
libtopology_la_SOURCES += topology-windows.c
else !HAVE_WINDOWS
EXTRA_DIST += topology-windows.c
endif !HAVE_WINDOWS
