# -*- shell-script -*-
#
# Copyright © 2014 Cisco Systems, Inc.  All rights reserved.
#
# $COPYRIGHT$
#
# See COPYING in top-level directory.
#
# Additional copyrights may follow
#
# $HEADER$
#
# This file is derived from configure.ac in the Jansson distribution.
# See the LICENSE file in the Jansson distribution for copyright and
# license information.
#
# NETLOC: The main differences between this derived file and the
# original configure.m4 are involved with making this file an m4 macro
# so that it can be called from the main netloc configure.ac (because
# netloc itself may be embedded, and AC_CONFIG_SUBDIR is not
# desirable).
#
# - Remove AC and AM INIT stuff
# - Remove PROG_CC and PROG_LIBTOOL
# - Add "jansson/" subdirs to CONFIG_HEADERS and CONFIG_FILES
# - Remove doc/ and test/ and *.pc files from CONFIG_FILES
# - Remove AC OUTPUT
# - Added "netloc/" prefix to the output files
# - Moved AM_CONDITIONALs to a separate macro that is always invoked
# - Remove GCC specific AM_CFLAGS definition, the last flag isn't
#   always supported and we don't want to pollute AM_CFLAGS everywhere
#   (only the removal part of fix for https://github.com/akheron/jansson/issues/203)
#
# Note that there were a few other changes; grep for "NETLOC:" in the
# jansson tree to find them (e.g., Makefile.am and src/Makefile.am).
#

# Need to be able to invoke Jansson's CONDITIONALs unconditionally
AC_DEFUN([JANSSON_DO_AM_CONDITIONALS],[
     # Checks for programs.
     AM_CONDITIONAL([GCC], [test x$GCC = xyes])
])dnl

AC_DEFUN([JANSSON_CONFIG],[

# NETLOC: prefixed this file with "netloc/"
AC_CONFIG_HEADERS(netloc_config_prefix[netloc/jansson/config.h])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([endian.h fcntl.h locale.h sched.h unistd.h sys/param.h sys/stat.h sys/time.h sys/types.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_LONG_LONG_INT

AC_C_INLINE
case $ac_cv_c_inline in
    yes) json_inline=inline;;
    no) json_inline=;;
    *) json_inline=$ac_cv_c_inline;;
esac
AC_SUBST([json_inline])

# Checks for library functions.
AC_CHECK_FUNCS([close getpid gettimeofday localeconv open read sched_yield strtoll])

AC_MSG_CHECKING([for gcc __sync builtins])
have_sync_builtins=no
AC_TRY_LINK(
  [], [unsigned long val; __sync_bool_compare_and_swap(&val, 0, 1);],
  [have_sync_builtins=yes],
)
if test "x$have_sync_builtins" = "xyes"; then
  AC_DEFINE([HAVE_SYNC_BUILTINS], [1],
    [Define to 1 if gcc's __sync builtins are available])
fi
AC_MSG_RESULT([$have_sync_builtins])

AC_MSG_CHECKING([for gcc __atomic builtins])
have_atomic_builtins=no
AC_TRY_LINK(
  [], [char l; unsigned long v; __atomic_test_and_set(&l, __ATOMIC_RELAXED); __atomic_store_n(&v, 1, __ATOMIC_ACQ_REL); __atomic_load_n(&v, __ATOMIC_ACQUIRE);],
  [have_atomic_builtins=yes],
)
if test "x$have_atomic_builtins" = "xyes"; then
  AC_DEFINE([HAVE_ATOMIC_BUILTINS], [1],
    [Define to 1 if gcc's __atomic builtins are available])
fi
AC_MSG_RESULT([$have_atomic_builtins])

case "$ac_cv_type_long_long_int$ac_cv_func_strtoll" in
     yesyes) json_have_long_long=1;;
     *) json_have_long_long=0;;
esac
AC_SUBST([json_have_long_long])

case "$ac_cv_header_locale_h$ac_cv_func_localeconv" in
     yesyes) json_have_localeconv=1;;
     *) json_have_localeconv=0;;
esac
AC_SUBST([json_have_localeconv])

# Features
AC_ARG_ENABLE([urandom],
  [AS_HELP_STRING([--disable-urandom],
    [Don't use /dev/urandom to seed the hash function])],
  [use_urandom=$enableval], [use_urandom=yes])

if test "x$use_urandom" = xyes; then
AC_DEFINE([USE_URANDOM], [1],
  [Define to 1 if /dev/urandom should be used for seeding the hash function])
fi

AC_ARG_ENABLE([windows-cryptoapi],
  [AS_HELP_STRING([--disable-windows-cryptoapi],
    [Don't use CryptGenRandom to seed the hash function])],
  [use_windows_cryptoapi=$enableval], [use_windows_cryptoapi=yes])

if test "x$use_windows_cryptoapi" = xyes; then
AC_DEFINE([USE_WINDOWS_CRYPTOAPI], [1],
  [Define to 1 if CryptGenRandom should be used for seeding the hash function])
fi

# NETLOC: prefixed these files with "netloc/"
AC_CONFIG_FILES(
        netloc_config_prefix[netloc/jansson/Makefile]
        netloc_config_prefix[netloc/jansson/src/Makefile]
        netloc_config_prefix[netloc/jansson/src/jansson_config.h]
)


]) dnl JANSSON_CONFIG defun
