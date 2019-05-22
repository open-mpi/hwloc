AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

noinst_LTLIBRARIES = libcpuaffinity.la
libcpuaffinity_la_SOURCES=tleaf.c utils.c enum.c
if HWLOC_HAVE_PTRACE
libcpuaffinity_la_SOURCES+=bind.c
endif
LIBADD=$(HWLOC_top_builddir)/hwloc/libhwloc.la

bin_PROGRAMS=hwloc-affinity
hwloc_affinity_SOURCES=hwloc-affinity.c
LDADD=$(HWLOC_top_builddir)/hwloc/libhwloc.la libcpuaffinity.la

###################################################
# Man
###################################################

man1_pages = hwloc-affinity.1
EXTRA_DIST = $(man1_pages:.1=.1in)
nodist_man_MANS = $(man1_pages)

SEDMAN = $(SED) -e 's/%PACKAGE_NAME%/@PACKAGE_NAME@/g' \
	  -e 's/%PACKAGE_VERSION%/@PACKAGE_VERSION@/g' \
	  -e 's/%HWLOC_DATE%/@HWLOC_RELEASE_DATE@/g'

.1in.1:
	@ echo Creating $@ man page...
	@ $(SEDMAN) \
	  > $@ < $<

###################################################
# Tests
###################################################

AM_COLOR_TESTS=yes

AM_CFLAGS+=@OPENMP_CFLAGS@
AM_CFLAGS+=@PTHREAD_CFLAGS@
LDADD+=@PTHREAD_LIBS@

UNIT_TESTS=test_tleaf test_enum test_bind
check_PROGRAMS=$(UNIT_TESTS)
TESTS=$(UNIT_TESTS)

EXTRA_DIST+=knl.xml
