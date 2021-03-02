#!/bin/sh
#
# Copyright © 2012-2021 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

echo "############################"
echo "Running on:"
uname -a
echo "Options: $0"
echo "############################"

set -e
set -x

tarball="$1"
arch="$2"

if test "x$arch" = "x32"; then
  # check the mingw shell
  if test "x$MINGW_CHOST" != xi686-w64-mingw32; then
    echo "MINGW_CHOST is $MINGW_CHOST instead of i686-w64-mingw32."
    exit 1
  fi
  # for MS 'lib' program in dolib.c
  export PATH=$PATH:$MSLIB32_PATH
else if test "x$arch" = "x64"; then
  # check the mingw shell
  if test "x$MINGW_CHOST" != xx86_64-w64-mingw32; then
    echo "MINGW_CHOST is $MINGW_CHOST instead of x86_64-w64-mingw32."
    exit 1
  fi
  # for MS 'lib' program in dolib.c
  export PATH=$PATH:$MSLIB64_PATH
else
  echo "Architecture parameter must be 32 or 64."
  exit 1
fi
fi

# check that we have a lib.exe
if ! which lib.exe ; then
  echo "Couldn't find MSVC lib.exe"
  exit 1
fi

# remove previous artifacts so that they aren't exported again by this build
rm -f hwloc-win*-build-*.zip || true

# extract the tarball
if ! test -f "$tarball"; then
  echo "Invalid tarball parameter."
  exit 1
fi
basename=$(basename $tarball .tar.gz)
version=$(echo $basename | cut -d- -f2-)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball

mkdir ${basename}/build || true
cd ${basename}/build

winball=hwloc-win${arch}-build-${version}
prefix=${PWD}/../${winball}
../configure --prefix=$prefix --enable-static CC="gcc -static-libgcc" $confopts

make

if test x$NO_CHECK != xtrue; then
  make check
fi

make install

cd ..

zip -r ../${winball}.zip ${winball}
test -f ${winball}/lib/libhwloc.lib || false

build/utils/lstopo/lstopo-no-graphics -v
build/utils/lstopo/lstopo-no-graphics --windows-processor-groups
build/utils/hwloc/hwloc-info --support

cd ..

rm -f $tarball

exit 0
