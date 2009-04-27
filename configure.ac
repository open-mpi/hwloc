#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([libtopology], [0.0], [libtopology-commits@gforge.inria.fr])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_SYSTEM
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
  *)
    echo Host type \"${target}\" not yet supported.
    exit 1;
    ;;
esac

AM_CONDITIONAL([HAVE_LINUX], [test "x$linux" = "xyes"])
AM_CONDITIONAL([HAVE_IRIX], [test "x$irix" = "xyes"])
AM_CONDITIONAL([HAVE_DARWIN], [test "x$darwin" = "xyes"])
AM_CONDITIONAL([HAVE_SOLARIS], [test "x$solaris" = "xyes"])
AM_CONDITIONAL([HAVE_AIX], [test "x$aix" = "xyes"])
AM_CONDITIONAL([HAVE_OSF], [test "x$osf" = "xyes"])

AC_CHECK_HEADERS([sys/lgrp_user.h], [
  AC_HAVE_LIBRARY([lgrp])
])
AC_CHECK_HEADERS([kstat.h], [
  AC_HAVE_LIBRARY([kstat])
])

PKG_CHECK_MODULES([CAIRO], [cairo], [
  AC_DEFINE([HAVE_CAIRO], [1], [Define to 1 if you have the `cairo' library.])
],[echo no])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/topology.c])
AC_CONFIG_HEADER([include/config.h])

AC_LANG([C])

AC_GNU_SOURCE # for O_DIRECTORY, fdopen, ffsl, ...

AC_PROG_CC_C99

# disable C++, F77, Java and Windows Resource Compiler support
# change this into LT_LANG+LT_INIT when we'll switch to libtool2
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
m4_defun([_LT_AC_LANG_GCJ_CONFIG], [:])
m4_defun([_LT_AC_LANG_RC_CONFIG], [:])
AC_PROG_LIBTOOL

TOPO_GCC_FLAGS

AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug], [enable debugging messages]),
  [enable_debug="$enableval"],
  [enable_debug="no"])

AC_MSG_CHECKING([whether debug is enabled])
if test x$enable_debug = xyes; then
  AC_DEFINE_UNQUOTED([LT_DEBUG], [1], [Define to 1 to enable debug])
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

AC_PATH_PROG([BASH], [bash])

AC_CHECK_FUNCS([ffs], [
  AC_CHECK_DECL([ffs],[
    AC_DEFINE([LT_HAVE_DECL_FFS], [1], [Define to 1 if function `ffs' is declared by system headers])
  ])
  AC_DEFINE([LT_HAVE_FFS], [1], [Define to 1 if you have the `ffs' function.])
])
AC_CHECK_FUNCS([ffsl], [
  AC_DEFINE([LT_HAVE_FFSL], [1], [Define to 1 if you have the `ffsl' function.])
])

AC_CONFIG_FILES([Makefile
                 include/Makefile src/Makefile utils/Makefile
		 tests/Makefile tests/linux/Makefile
		 tests/linux/test-topology.sh
		 doc/Makefile])

AC_CONFIG_HEADERS([include/libtopology/config.h])

AC_CONFIG_COMMANDS([linux-test-topology],
                   [chmod +x tests/linux/test-topology.sh])

AC_OUTPUT
