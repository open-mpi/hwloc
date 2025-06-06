# Copyright © 2009-2024 Inria.  All rights reserved.
# Copyright © 2009-2012, 2014 Université Bordeaux
# Copyright © 2009-2014 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2020 Hewlett Packard Enterprise.  All rights reserved.
# Copyright © 2023      Université de Reims Champagne-Ardenne.  All rights reserved.
#
# See COPYING in top-level directory.

# This makefile is only reached when building in standalone mode

AM_CFLAGS = $(HWLOC_CFLAGS)
# Add -I of utils/hwloc for misc.h
AM_CPPFLAGS = $(HWLOC_CPPFLAGS) -I$(top_srcdir)/utils/hwloc
AM_LDFLAGS = $(HWLOC_LDFLAGS)

LDADD = $(HWLOC_top_builddir)/hwloc/libhwloc.la \
	$(HWLOC_top_builddir)/utils/hwloc/libutils_common.la

bin_PROGRAMS = \
        lstopo-no-graphics

EXTRA_DIST = \
        lstopo-android.c \
        test-lstopo.output

lstopo_no_graphics_SOURCES = \
        lstopo.h \
        lstopo.c \
        lstopo-draw.c \
        lstopo-tikz.c \
        lstopo-fig.c \
        lstopo-svg.c \
        lstopo-ascii.c \
        lstopo-text.c \
        lstopo-xml.c

if !HWLOC_HAVE_WINDOWS
lstopo_no_graphics_SOURCES += lstopo-shmem.c
endif

lstopo_no_graphics_CFLAGS = $(AM_CFLAGS)
lstopo_no_graphics_LDADD = $(LDADD) -lm $(HWLOC_TERMCAP_LIBS)

lstopo_SOURCES = $(lstopo_no_graphics_SOURCES)
lstopo_CPPFLAGS = $(AM_CPPFLAGS)
lstopo_LDADD = $(lstopo_no_graphics_LDADD)
if HWLOC_HAVE_CAIRO
bin_PROGRAMS += lstopo
lstopo_SOURCES += lstopo-cairo.c
lstopo_CPPFLAGS += -DLSTOPO_HAVE_GRAPHICS $(HWLOC_X11_CPPFLAGS)
lstopo_CFLAGS = $(lstopo_no_graphics_CFLAGS) $(HWLOC_CAIRO_CFLAGS)
lstopo_LDADD += $(HWLOC_CAIRO_LIBS) $(HWLOC_X11_LIBS)
endif
if HWLOC_HAVE_WINDOWS
bin_PROGRAMS += lstopo
if !HWLOC_HAVE_MSVC
# only build lstopo-win if NOT building with MSVC (or Clang in MSVC mode) because
# MSVC doesn't support -mwindows, and lstopo-win would be identical to lstopo
bin_PROGRAMS += lstopo-win
endif
lstopo_SOURCES += lstopo-windows.c
lstopo_CPPFLAGS += -DLSTOPO_HAVE_GRAPHICS
if HWLOC_HAVE_USER32
lstopo_LDADD += -luser32
endif
lstopo_win_SOURCES = $(lstopo_SOURCES)
lstopo_win_CPPFLAGS = $(lstopo_CPPFLAGS)
lstopo_win_CFLAGS = $(lstopo_CFLAGS)
if !HWLOC_HAVE_MSVC
lstopo_win_CFLAGS += -mwindows
endif
lstopo_win_LDADD = $(lstopo_LDADD)
endif

man1_pages = lstopo-no-graphics.1
EXTRA_DIST += $(man1_pages:.1=.1in)
nodist_man_MANS = $(man1_pages)

TESTS = \
        test-lstopo.sh
if HWLOC_HAVE_LINUX
TESTS += test-lstopo-shmem.sh
endif HWLOC_HAVE_LINUX

if HWLOC_HAVE_CAIRO
# only installed when lstopo is built with Cairo/X11 support
# so that no terminal is required
APPLICATIONSdir = @datarootdir@/applications
dist_APPLICATIONS_DATA = lstopo.desktop
endif

SEDMAN = $(SED) -e 's/%PACKAGE_NAME%/@PACKAGE_NAME@/g' \
	  -e 's/%PACKAGE_VERSION%/@PACKAGE_VERSION@/g' \
	  -e 's/%HWLOC_DATE%/@HWLOC_RELEASE_DATE@/g'

.1in.1:
	@ echo Creating $@ man page...
	@ $(SEDMAN) \
	  > $@ < $<

hwloc_ls_name=`echo "hwloc-ls" | $(SED) -e "$(transform)"`
lstopo_name=`echo "lstopo" | $(SED) -e "$(transform)"`
lstopo_no_graphics_name=`echo "lstopo-no-graphics" | $(SED) -e "$(transform)"`

install-exec-hook:
	rm -f $(DESTDIR)$(bindir)/$(hwloc_ls_name)$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) $(lstopo_no_graphics_name)$(EXEEXT) $(hwloc_ls_name)$(EXEEXT)
if !HWLOC_HAVE_WINDOWS
if !HWLOC_HAVE_CAIRO
	rm -f $(DESTDIR)$(bindir)/$(lstopo_name)$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) $(lstopo_no_graphics_name)$(EXEEXT) $(lstopo_name)$(EXEEXT) || true
endif
endif

install-data-hook:
	rm -f $(DESTDIR)$(man1dir)/$(hwloc_ls_name).1
	cd $(DESTDIR)$(man1dir) && $(LN_S) $(lstopo_no_graphics_name).1 $(hwloc_ls_name).1
	rm -f $(DESTDIR)$(man1dir)/$(lstopo_name).1
	cd $(DESTDIR)$(man1dir) && $(LN_S) $(lstopo_no_graphics_name).1 $(lstopo_name).1

uninstall-local:
	rm -f $(DESTDIR)$(bindir)/$(hwloc_ls_name)$(EXEEXT)
if !HWLOC_HAVE_WINDOWS
if !HWLOC_HAVE_CAIRO
	rm -f $(DESTDIR)$(bindir)/$(lstopo_name)$(EXEEXT)
endif
endif
	rm -f $(DESTDIR)$(man1dir)/$(hwloc_ls_name).1 $(DESTDIR)$(man1dir)/$(lstopo_name).1

distclean-local:
	rm -f $(nodist_man_MANS)
