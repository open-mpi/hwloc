AM_CFLAGS = $(HWLOC_CFLAGS) -I$(HWLOC_top_builddir)/utils/hwloc/hwloc-affinity
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

noinst_LTLIBRARIES = libhwloc-tbind.la
libhwloc_tbind_la_SOURCES=hwloc-tbind-obj.c
LIBDADD=$(HWLOC_top_builddir)/hwloc/libhwloc.la

bin_PROGRAMS=hwloc-tbind
hwloc_tbind_SOURCES=hwloc-tbind.c
LDADD=$(HWLOC_top_builddir)/hwloc/libhwloc.la
LDADD+=libhwloc-tbind.la

###################################################
# Man
###################################################

man1_pages = hwloc-tbind.1
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

AM_CFLAGS+=-DXML_DIR=\"$(HWLOC_top_builddir)/tests/hwloc/xml\"
AM_CFLAGS+=@OPENMP_CFLAGS@
AM_CFLAGS+=@PTHREAD_CFLAGS@

UNIT_TESTS=test-hwloc-tbind
check_PROGRAMS=$(UNIT_TESTS)
TESTS=$(UNIT_TESTS)
