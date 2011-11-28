dnl -*- Autoconf -*-
dnl Copyright © 2009 CNRS
dnl Copyright © 2009 INRIA.  All rights reserved.
dnl Copyright © 2009 Université Bordeaux 1
dnl See COPYING in top-level directory.

dnl HWLOC_CHECK_DECL
dnl
dnl Check declaration of given function by trying to call it with an insane
dnl number of arguments (10). Success means the compiler couldn't really check.
AC_DEFUN([HWLOC_CHECK_DECL], [
  AC_MSG_CHECKING([whether function $1 is declared])
  AC_REQUIRE([AC_PROG_CC])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([$4])],[$1(1,2,3,4,5,6,7,8,9,10);])],
    ac_res=no
    $3,
    ac_res=yes
    $2
  )
  AC_MSG_RESULT([$ac_res])
])

dnl HWLOC_CHECK_DECLS
dnl
dnl Same as HWLOCK_CHECK_DECL, but defines HAVE_DECL_foo to 1 or 0 depending on
dnl the result.
AC_DEFUN([HWLOC_CHECK_DECLS], [
  HWLOC_CHECK_DECL([$1], [ac_have_decl=1], [ac_have_decl=0], [$4])
  AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_DECL_$1]), [$ac_have_decl],
    [Define to 1 if you have the declaration of `$1', and to 0 if you don't])
])
