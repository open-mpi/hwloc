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
  *)
    linux=no
    echo Host type \"${target}\" not yet supported.
    exit 1;
    ;;
esac

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/topology.c])
AC_CONFIG_HEADER([include/config.h])

AM_CONDITIONAL([HAVE_LINUX], [test "x$linux" = "xyes"])

AC_LANG([C])

AC_GNU_SOURCE # for O_DIRECTORY, fdopen, ffsl, ...

AC_PROG_CC_C89
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

AC_CONFIG_FILES([Makefile
                 include/Makefile src/Makefile utils/Makefile
		 tests/Makefile tests/linux/Makefile
		 tests/linux/test-topology.sh
		 doc/Makefile])

AC_CONFIG_COMMANDS([linux-test-topology],
                   [chmod +x tests/linux/test-topology.sh])

AC_OUTPUT
