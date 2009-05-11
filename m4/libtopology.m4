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
