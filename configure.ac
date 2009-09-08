#
# Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
# See COPYING in top-level directory.
#

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
# Initialize autostuff
#

AC_PREREQ([2.60])
AC_INIT([libtopology], [0.9], [libtopology-devel@gforge.inria.fr])
AC_SUBST([TOPO_SOVERSION],[0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/topology.c])
DX_MAN_FEATURE(ON)
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(ON)
DX_PS_FEATURE(OFF)
if echo "${srcdir}" | grep '^/' > /dev/null
then
  # absolute path, no need for ../
  DX_INIT_DOXYGEN(libtopology, doxygen.cfg, ${srcdir}/doc/doxygen-doc)
else
  DX_INIT_DOXYGEN(libtopology, doxygen.cfg, ../${srcdir}/doc/doxygen-doc)
fi
AC_PATH_PROG([FIG2DEV], [fig2dev])
AC_CHECK_FILE(${srcdir}/doc/doxygen-doc/man, [topo_have_man=yes], [:])
AC_CHECK_FILE(${srcdir}/doc/doxygen-doc/libtopology.pdf, [topo_have_pdf=yes], [:])
AC_CHECK_FILE(${srcdir}/doc/doxygen-doc/html, [topo_have_html=yes], [:])
AC_CHECK_FILE(${srcdir}/doc/doxygen-doc/libtopology.tag, [topo_have_tag=yes], [:])
AM_CONDITIONAL([TOPO_HAVE_MAN], [test "x$topo_have_man" = "xyes"])
AM_CONDITIONAL([TOPO_HAVE_PDF], [test "x$topo_have_pdf" = "xyes"])
AM_CONDITIONAL([TOPO_HAVE_HTML], [test "x$topo_have_html" = "xyes"])
AM_CONDITIONAL([TOPO_HAVE_DOC], [test "x$topo_have_html" = "xyes"])
AM_CONDITIONAL([TOPO_HAVE_TAG], [test "x$topo_have_tag" = "xyes"])
AM_CONDITIONAL([TOPO_INSTALL_MAN], [test "x$topo_have_man" = "xyes" -o "x$DX_FLAG_man" = "x1" ])
AM_CONDITIONAL([TOPO_INSTALL_PDF], [test "x$topo_have_pdf" = "xyes" -o \( "x$DX_FLAG_pdf" = "x1" -a "x$FIG2DEV" != "x" \) ])
AM_CONDITIONAL([TOPO_INSTALL_HTML], [test "x$topo_have_html" = "xyes" -o \( "x$DX_FLAG_html" = "x1" -a "x$FIG2DEV" != "x" \) ])
AM_CONDITIONAL([TOPO_WILLHAVE_TAG], [test "x$topo_have_tag" = "xyes" -o \( "x$DX_DOXYGEN" != "x" -a "x$FIG2DEV" != "x" \) ])
AM_INIT_AUTOMAKE([1.10 foreign -Wall -Werror])

#
# Check OS support
#

case ${target} in
  *-*-linux*)
    AC_DEFINE(LINUX_SYS, 1, Define to 1 on Linux)
    linux=yes
    ;;
  *-*-irix*)
    AC_DEFINE(IRIX_SYS, 1, Define to 1 on Irix)
    irix=yes
    ;;
  *-*-darwin*)
    AC_DEFINE(DARWIN_SYS, 1, Define to 1 on Darwin)
    darwin=yes
    ;;
  *-*-solaris*)
    AC_DEFINE(SOLARIS_SYS, 1, Define to 1 on Solaris)
    solaris=yes
    ;;
  *-*-aix*)
    AC_DEFINE(AIX_SYS, 1, Define to 1 on AIX)
    aix=yes
    ;;
  *-*-osf*)
    AC_DEFINE(OSF_SYS, 1, Define to 1 on OSF)
    osf=yes
    ;;
  *-*-hpux*)
    AC_DEFINE(HPUX_SYS, 1, Define to 1 on HP-UX)
    hpux=yes
    ;;
  *-*-mingw*|*-*-cygwin*)
    AC_DEFINE(WIN_SYS, 1, Define to 1 on WINDOWS)
    windows=yes
    ;;
  *)
    echo Warning: Host type \"${target}\" not yet supported.
    ;;
esac

AM_CONDITIONAL([HAVE_LINUX], [test "x$linux" = "xyes"])
AM_CONDITIONAL([HAVE_IRIX], [test "x$irix" = "xyes"])
AM_CONDITIONAL([HAVE_DARWIN], [test "x$darwin" = "xyes"])
AM_CONDITIONAL([HAVE_SOLARIS], [test "x$solaris" = "xyes"])
AM_CONDITIONAL([HAVE_AIX], [test "x$aix" = "xyes"])
AM_CONDITIONAL([HAVE_OSF], [test "x$osf" = "xyes"])
AM_CONDITIONAL([HAVE_HPUX], [test "x$hpux" = "xyes"])
AM_CONDITIONAL([HAVE_WINDOWS], [test "x$windows" = "xyes"])
AM_CONDITIONAL([HAVE_MINGW32], [test "x$target_os" = "xmingw32"])

#
# Define C flags
#

AC_USE_SYSTEM_EXTENSIONS # for O_DIRECTORY, fdopen, ffsl, ...

AC_LANG([C])

AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_MKDIR_P
PKG_CHECK_EXISTS

TOPO_GCC_FLAGS
TOPO_CHECK_DIFF_U

AC_CHECK_SIZEOF([unsigned long])
AC_DEFINE_UNQUOTED([TOPO_SIZEOF_UNSIGNED_LONG], $ac_cv_sizeof_unsigned_long, [The size of `unsigned long', as computed by sizeof])
AC_CHECK_SIZEOF([unsigned int])
AC_DEFINE_UNQUOTED([TOPO_SIZEOF_UNSIGNED_INT], $ac_cv_sizeof_unsigned_int, [The size of `unsigned int', as computed by sizeof])

#
# Now detect support
#

AC_CHECK_TYPES([wchar_t], [
  AC_CHECK_FUNCS([putwc])
], [], [[#include <wchar.h>]])

AC_CHECK_HEADERS([locale.h], [
  AC_CHECK_FUNCS([setlocale])
])
AC_CHECK_HEADERS([langinfo.h], [
  AC_CHECK_FUNCS([nl_langinfo])
])
AC_CHECK_HEADERS([curses.h], [
  AC_CHECK_HEADERS([term.h], [
    AC_CHECK_LIB([termcap], [tparm])
  ], [], [[#include <curses.h>]])
])

AC_CHECK_TYPES([KAFFINITY,
    		    PROCESSOR_CACHE_TYPE,
		    CACHE_DESCRIPTOR,
		    LOGICAL_PROCESSOR_RELATIONSHIP,
		    SYSTEM_LOGICAL_PROCESSOR_INFORMATION,
		    GROUP_AFFINITY,
		    PROCESSOR_RELATIONSHIP,
		    NUMA_NODE_RELATIONSHIP,
		    CACHE_RELATIONSHIP,
		    PROCESSOR_GROUP_INFO,
		    GROUP_RELATIONSHIP,
		    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX],
		    [],[],[[#include <windows.h>]])
AC_HAVE_LIBRARY(gdi32)

AC_CHECK_HEADER([windows.h], [
  AC_DEFINE([TOPO_HAVE_WINDOWS_H], [1], [Define to 1 if you have the `windows.h' header.])
])

AC_CHECK_HEADERS([sys/lgrp_user.h], [
  AC_HAVE_LIBRARY([lgrp])
])
AC_CHECK_HEADERS([kstat.h], [
  AC_HAVE_LIBRARY([kstat])
])

AC_CHECK_DECLS([_SC_NPROCESSORS_ONLN,
		_SC_NPROCESSORS_CONF,
		_SC_NPROC_ONLN,
		_SC_NPROC_CONF,
		_SC_LARGE_PAGESIZE],,[:],[[#include <unistd.h>]])

AC_HAVE_HEADERS([mach/mach_host.h])
AC_HAVE_HEADERS([mach/mach_init.h], [
  AC_CHECK_FUNCS([host_info])
])

case ${target} in
  *-*-mingw*|*-*-cygwin*)
    topo_pid_t=HANDLE
    topo_thread_t=HANDLE
    ;;
  *)
    topo_pid_t=pid_t
    AC_CHECK_TYPES([pthread_t], [topo_thread_t=pthread_t], [:], [[#include <pthread.h>]])
    ;;
esac
AC_DEFINE_UNQUOTED(topo_pid_t, $topo_pid_t, [Define this to the process ID type])
if test "x$topo_thread_t" != "x" ; then
  AC_DEFINE_UNQUOTED(topo_thread_t, $topo_thread_t, [Define this to the thread ID type])
fi

AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[
#define _GNU_SOURCE
#include <sched.h>
static unsigned long mask;
]], [[ sched_setaffinity(0, (void*) &mask);
]]), AC_DEFINE([HAVE_OLD_SCHED_SETAFFINITY], [1], [Define to 1 if glibc provides the old prototype of sched_setaffinity()]))

AC_ARG_ENABLE([cairo],
  [AS_HELP_STRING([--disable-cairo], [disable the Cairo back-end of `lstopo'])],
  [enable_cairo="$enableval"],
  [enable_cairo="yes"])

if test "x$enable_cairo" = "xyes"; then
  PKG_CHECK_MODULES([CAIRO], [cairo], [:], [enable_cairo="no"])
  if test "x$enable_cairo" = "xyes"; then
    AC_CHECK_HEADERS([X11/Xlib.h],
      [AC_CHECK_HEADERS([X11/Xutil.h X11/keysym.h],
        [enable_X11=yes
         AC_DEFINE([HAVE_X11], [1], [Define to 1 if X11 libraries are available.])],,
        [[#include <X11/Xlib.h>]])
      ])
    if test "x$enable_X11" != "xyes"; then
      AC_MSG_WARN([X11 headers not found, Cairo/X11 back-end disabled])
    fi
  fi
fi

if test "x$enable_cairo" = "xyes"; then
  AC_DEFINE([HAVE_CAIRO], [1], [Define to 1 if you have the `cairo' library.])
fi

AC_ARG_ENABLE([xml],
  [AS_HELP_STRING([--disable-xml], [disable the XML back-end of `lstopo'])],
  [enable_xml="$enableval"],
  [enable_xml="yes"])

if test "x$enable_xml" = "xyes"; then
  PKG_CHECK_MODULES([XML], [libxml-2.0], [:], [enable_xml="no"])
fi

if test "x$enable_xml" = "xyes"; then
  LIBTOPOLOGY_REQUIRES="libxml-2.0 $LIBTOPOLOGY_REQUIRES"
  AC_DEFINE([HAVE_XML], [1], [Define to 1 if you have the `xml' library.])
  AC_SUBST([HAVE_XML], [1])
else
  AC_SUBST([HAVE_XML], [0])
fi
AM_CONDITIONAL([HAVE_XML], [test "x$enable_xml" = "xyes"])

# check for kerrighed, but don't abort if not found
PKG_CHECK_MODULES([KERRIGHED], [kerrighed >= 2.0], [], [:])

# disable C++, F77, Java and Windows Resource Compiler support
LT_PREREQ([2.2.6])
LT_INIT
LT_LANG([C])
AC_LIBTOOL_WIN32_DLL
AM_CONDITIONAL([HAVE_GCC], [test "x$GCC" = "xyes"])
AC_PATH_PROGS([TOPO_MS_LIB], [lib])
AC_ARG_VAR([TOPO_MS_LIB], [Path to Microsoft's Visual Studio `lib' tool])
AM_CONDITIONAL([HAVE_MS_LIB], [test "x$TOPO_MS_LIB" != "x"])

AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug], [enable debugging messages]),
  [enable_debug="$enableval"],
  [enable_debug="no"])

AC_MSG_CHECKING([whether debug is enabled])
if test x$enable_debug = xyes; then
  AC_DEFINE_UNQUOTED([TOPO_DEBUG], [1], [Define to 1 to enable debug])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

AC_PATH_PROG([BASH], [bash])
AC_PATH_PROG([W3M], [w3m])

AC_PATH_PROG([HELP2MAN], [help2man])
AC_CHECK_FILE(${srcdir}/utils/lstopo.1, [topo_have_utils_man=yes], [:])
AM_CONDITIONAL([TOPO_HAVE_UTILS_MAN], [test "x$topo_have_utils_man" = "xyes"])
AM_CONDITIONAL([TOPO_INSTALL_UTILS_MAN], [test "x$topo_have_utils_man" = "xyes" -o "x$HELP2MAN" != "x"])

AC_CHECK_FUNCS([ffs], [
  AC_CHECK_DECL([ffs],[
    AC_DEFINE([TOPO_HAVE_DECL_FFS], [1], [Define to 1 if function `ffs' is declared by system headers])
  ])
  AC_DEFINE([TOPO_HAVE_FFS], [1], [Define to 1 if you have the `ffs' function.])
])
AC_CHECK_FUNCS([ffsl], [
  AC_DEFINE([TOPO_HAVE_FFSL], [1], [Define to 1 if you have the `ffsl' function.])
])

AC_CHECK_FUNCS([fls], [
  AC_CHECK_DECL([fls],[
    AC_DEFINE([TOPO_HAVE_DECL_FLS], [1], [Define to 1 if function `fls' is declared by system headers])
  ])
  AC_DEFINE([TOPO_HAVE_FLS], [1], [Define to 1 if you have the `fls' function.])
])
AC_CHECK_FUNCS([flsl], [
  AC_DEFINE([TOPO_HAVE_FLSL], [1], [Define to 1 if you have the `flsl' function.])
])

AC_CHECK_FUNCS([clz], [
  AC_CHECK_DECL([clz],[
    AC_DEFINE([TOPO_HAVE_DECL_CLZ], [1], [Define to 1 if function `clz' is declared by system headers])
  ])
  AC_DEFINE([TOPO_HAVE_CLZ], [1], [Define to 1 if you have the `clz' function.])
])
AC_CHECK_FUNCS([clzl], [
  AC_DEFINE([TOPO_HAVE_CLZL], [1], [Define to 1 if you have the `clzl' function.])
])

AC_CHECK_FUNCS([openat], [have_openat=yes])
AM_CONDITIONAL([HAVE_OPENAT], [test "x$have_openat" = "xyes"])

AC_FUNC_ALLOCA

AC_CHECK_DECL([numa_bitmask_alloc], [have_linux_libnuma=yes], [],
	      [#include <numa.h>])
AM_CONDITIONAL([HAVE_LINUX_LIBNUMA], [test "x$have_linux_libnuma" = "xyes"])

AC_CHECK_DECLS([pthread_setaffinity_np],,[:],[[#include <pthread.h>]])
AC_CHECK_FUNC([sched_setaffinity], [have_sched_setaffinity=yes])
AM_CONDITIONAL([HAVE_SCHED_SETAFFINITY], [test "x$have_sched_setaffinity" = "xyes"])

AC_SUBST(LIBTOPOLOGY_REQUIRES)

AC_CONFIG_HEADER([include/private/config.h])

AC_CONFIG_FILES([Makefile
                 topology.pc
                 doc/doxygen.cfg
                 include/Makefile src/Makefile utils/Makefile
		 tests/Makefile tests/linux/Makefile
		 tests/linux/gather-topology.sh
		 tests/linux/test-topology.sh
		 tests/lstopo_xml.sh
		 utils/test-topodistrib.sh
		 tests/ports/Makefile
		 doc/Makefile])

AC_CONFIG_HEADERS([include/topology/config.h])

AC_CONFIG_COMMANDS([linux-topology-scripts],
                   [chmod +x tests/linux/test-topology.sh tests/lstopo_xml.sh tests/linux/gather-topology.sh utils/test-topodistrib.sh])

AC_OUTPUT
