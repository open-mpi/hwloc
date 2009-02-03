#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([libtopology], [0.0], [libtopology-commits@gforge.inria.fr])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/topology.c])
AC_CONFIG_HEADER([include/config.h])

AC_PROG_CC_C89
AC_PROG_LIBTOOL

TOPO_GCC_FLAGS

AC_CONFIG_FILES([Makefile
                 include/Makefile src/Makefile utils/Makefile tests/Makefile
		 doc/Makefile])

AC_OUTPUT
