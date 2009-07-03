dnl -*- Autoconf -*-
dnl
dnl Copyright 2009 INRIA, Université Bordeaux 1


dnl TOPO_GCC_FLAGS
dnl
dnl Substitute in `GCC_CFLAGS' GCC-specific flags.
AC_DEFUN([TOPO_GCC_FLAGS], [
  # GCC specifics.
  if test "x$GCC" = "xyes"; then
    GCC_CFLAGS="-std=c99 -Wall -Wmissing-prototypes -Wundef"
    GCC_CFLAGS="$GCC_CFLAGS -Wpointer-arith -Wcast-align"
  else
    GCC_CFLAGS=""
  fi
  AC_SUBST([GCC_CFLAGS])
])

AC_DEFUN([TOPO_CHECK_DIFF_U], [
  AC_MSG_CHECKING([whether diff accepts -u])
  if diff -u /dev/null /dev/null 2> /dev/null
  then
    TOPO_DIFF_U="-u"
  else
    TOPO_DIFF_U=""
  fi
  AC_SUBST([TOPO_DIFF_U])
  AC_MSG_RESULT([$TOPO_DIFF_U])
])
