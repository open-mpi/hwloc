#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
# Initialize autostuff
#

AC_PREREQ([2.60])
AC_INIT([libtopology], [0.0], [libtopology-commits@gforge.inria.fr])
AC_SUBST([TOPO_SOVERSION],[0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/topology.c])
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
AM_CONDITIONAL([HAVE_WINDOWS], [test "x$windows" = "xyes"])

#
# Define C flags
#

AC_USE_SYSTEM_EXTENSIONS # for O_DIRECTORY, fdopen, ffsl, ...

AC_LANG([C])

AC_PROG_CC_C99

TOPO_GCC_FLAGS

#
# Now detect support
#

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

AC_CHECK_HEADERS([sys/lgrp_user.h], [
  AC_HAVE_LIBRARY([lgrp])
])
AC_CHECK_HEADERS([kstat.h], [
  AC_HAVE_LIBRARY([kstat])
])

AC_ARG_ENABLE([cairo],
  [AS_HELP_STRING([--disable-cairo], [disable the Cairo back-end of `lstopo'])],
  [enable_cairo="$enableval"],
  [enable_cairo="yes"])

if test "x$enable_cairo" = "xyes"; then
  PKG_CHECK_MODULES([CAIRO], [cairo], [:], [enable_cairo="no"])
  if test "x$enable_cairo" = "xyes"; then
    AC_CHECK_HEADERS([X11/Xlib.h],
      [AC_CHECK_HEADERS([X11/Xutil.h],
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

# check for kerrighed, but don't abort if not found
PKG_CHECK_MODULES([KERRIGHED], [kerrighed >= 2.0], [], [:])

# disable C++, F77, Java and Windows Resource Compiler support
# change this into LT_LANG+LT_INIT when we'll switch to libtool2
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
m4_defun([_LT_AC_LANG_GCJ_CONFIG], [:])
m4_defun([_LT_AC_LANG_RC_CONFIG], [:])
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
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

AC_CHECK_FUNCS([ffs], [
  AC_CHECK_DECL([ffs],[
    AC_DEFINE([TOPO_HAVE_DECL_FFS], [1], [Define to 1 if function `ffs' is declared by system headers])
  ])
  AC_DEFINE([TOPO_HAVE_FFS], [1], [Define to 1 if you have the `ffs' function.])
])
AC_CHECK_FUNCS([ffsl], [
  AC_DEFINE([TOPO_HAVE_FFSL], [1], [Define to 1 if you have the `ffsl' function.])
])

AC_CONFIG_HEADER([include/config.h])

AC_CONFIG_FILES([Makefile
                 topology.pc
                 include/Makefile src/Makefile utils/Makefile
		 tests/Makefile tests/linux/Makefile
		 tests/linux/test-topology.sh
		 tests/ports/Makefile
		 doc/Makefile])

AC_CONFIG_HEADERS([include/topology/config.h])

AC_CONFIG_COMMANDS([linux-test-topology],
                   [chmod +x tests/linux/test-topology.sh])

AC_OUTPUT
