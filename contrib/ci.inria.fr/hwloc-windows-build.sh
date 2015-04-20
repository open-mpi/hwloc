#!/bin/sh

#
# Copyright © 20012-2015 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

check=1
build32=1
build64=1

while test $# -gt 0; do
  if test "$1" = "--no-check"; then
    check=0
  else if test "$1" = "--no-32"; then
    build32=0
  else if test "$1" = "--no-64"; then
    build64=0
  else if test "$1" = "--help"; then
    echo "  --no-check"
    echo "  --no-32"
    echo "  --no-64"
    echo "  --help"
  fi fi fi fi
  shift
done

oldPATH=$PATH

tarball=$(ls -tr hwloc-*.tar.gz | tail -1)
basename=$(basename $tarball .tar.gz)
version=$(echo $basename | cut -d- -f2)

test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball

if test x$build32 = x1; then

mkdir ${basename}/build32
cd ${basename}/build32
winball=hwloc-win32-build-${version}
prefix=${PWD}/../${winball}
export PATH=/c/Builds:/c/Builds/mingw32/bin/:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
../configure --prefix=$prefix --enable-static --host=i686-w64-mingw32 CC_FOR_BUILD=x86_64-w64-mingw32-gcc
make
make install
#make install-winball || true # not needed anymore in v1.7+
if test x$check = x1; then
  make check
fi
utils/lstopo/lstopo-no-graphics -v
cd ..
zip -r ../${winball}.zip ${winball}
test -f ${winball}/lib/libhwloc.lib || false
cd ..

fi


if test x$build64 = x1; then

mkdir ${basename}/build64
cd ${basename}/build64
winball=hwloc-win64-build-${version}
prefix=${PWD}/../${winball}
export PATH=/c/Builds:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib/:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
../configure --prefix=$prefix --enable-static --host=x86_64-w64-mingw32
make
make install
#make install-winball || true # not needed anymore in v1.7+
if test x$check = x1; then
  make check
fi
utils/lstopo/lstopo-no-graphics -v
cd ..
zip -r ../${winball}.zip ${winball}
test -f ${winball}/lib/libhwloc.lib || false
cd ..

fi

PATH=$oldPATH
