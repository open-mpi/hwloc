#!/usr/bin/env bash

srcdir=$(dirname "$0")
if test "x$srcdir" != x; then
   # in case we ever autogen on a platform without dirname
  cd $srcdir
fi

autoreconf ${autoreconf_args:-"-ivf"}

echo -n "Checking whether configure needs patching for MacOS Big Sur libtool.m4 bug... "
if grep -A1 MACOSX_DEPLOYMENT_TARGET configure | grep powerpc >/dev/null \
   || grep -A1 MACOSX_DEPLOYMENT_TARGET configure | grep 'darwin\[912' >/dev/null; then
  echo "no"
else
  echo "yes"
  echo "Trying to patch configure..."
  if patch -p1 --dry-run < config/libtool-big-sur-fixup.patch; then
     echo "Patching for real now"
     patch -p1 < config/libtool-big-sur-fixup.patch
  else
     echo "WARNING: Couldn't apply Big Sur libtool.m4 bug fix."
  fi
fi
