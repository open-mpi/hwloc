# Copyright © 2009-2014 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013 Inria.  All rights reserved.
# See COPYING in top-level directory.

AC_INIT([netloc-embedded-test], [1.0], [http://www.open-mpi.org/projects/hwloc/], [netloc-embedded-test])
AC_PREREQ([2.63])
AC_CONFIG_AUX_DIR([./config])
AC_CONFIG_MACRO_DIR([./config])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([1.10 dist-bzip2 foreign subdir-objects tar-ustar -Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_LANG([C])

AC_PROG_CC
AM_PROG_CC_C_O

cat <<EOF

=== Configuring netloc (embedded mode)
EOF

HWLOC_SET_SYMBOL_PREFIX([mytest_])
HWLOC_SETUP_CORE([netloc-tree], [netloc_setup=happy], [netloc_setup=unhappy])
AS_IF([test "$netloc_setup" = "unhappy"],
      [AC_MSG_ERROR([Cannot build hwloc core])])
NETLOC_SETUP_CORE([netloc-tree], [netloc_setup=happy], [netloc_setup=unhappy])
AS_IF([test "$netloc_setup" = "unhappy"],
      [AC_MSG_ERROR([Cannot build netloc core])])
NETLOC_DO_AM_CONDITIONALS
HWLOC_DO_AM_CONDITIONALS

cat <<EOF
=== Done configuring neloc (embedded mode)

EOF

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

AM_ENABLE_SHARED
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
