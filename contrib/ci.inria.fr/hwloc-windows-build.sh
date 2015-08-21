#!/bin/sh

#
# Copyright © 20012-2015 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e

docheck=1
dobuild32=1
dobuild64=1
dotar=1
doconf=1
confopts=

while test $# -gt 0; do
  if test "$1" = "--no-check"; then
    docheck=0
  else if test "$1" = "--no-32"; then
    dobuild32=0
  else if test "$1" = "--no-64"; then
    dobuild64=0
  else if test "$1" = "--no-tar"; then
    dotar=0
  else if test "$1" = "--no-conf"; then
    doconf=0
  else if test "$1" = "--debug"; then
    confopts="$confopts --enable-debug"
  else if test "$1" = "--help"; then
    echo "  --debug"
    echo "  --no-check"
    echo "  --no-32"
    echo "  --no-64"
    echo "  --no-tar"
    echo "  --no-conf"
    echo "  --help"
    exit 0
  fi fi fi fi fi fi fi
  shift
done

set -x

oldPATH=$PATH

if test x$dotar = x1; then
  tarball=$(ls -tr hwloc-*.tar.gz | tail -1)
  basename=$(basename $tarball .tar.gz)
  version=$(echo $basename | cut -d- -f2)

  test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
  tar xfz $tarball
else
  basename=./
  version=custom
fi

if test x$dobuild32 = x1; then

  mkdir ${basename}/build32 || true
  cd ${basename}/build32

  export PATH=/c/Builds:/c/Builds/mingw32/bin/:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
  if test x$doconf = x1; then
    winball=hwloc-win32-build-${version}
    prefix=${PWD}/../${winball}
    ../configure --prefix=$prefix --enable-static --host=i686-w64-mingw32 CC_FOR_BUILD=x86_64-w64-mingw32-gcc $confopts
  fi

  make
  make install
  #make install-winball || true # not needed anymore in v1.7+
  if test x$docheck = x1; then
    make check
  fi
  utils/lstopo/lstopo-no-graphics -v
  winball=$(basename $(head config.log | sed -r -n -e 's/.*--prefix=([^ ]+).*/\1/p'))
  cd ..

  zip -r ../${winball}.zip ${winball}
  test -f ${winball}/lib/libhwloc.lib || false
  if test x$dotar = x1; then
    cd ..
  fi

fi


if test x$dobuild64 = x1; then

  mkdir ${basename}/build64 || true
  cd ${basename}/build64

  export PATH=/c/Builds:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib/:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
  if test x$doconf = x1; then
    winball=hwloc-win64-build-${version}
    prefix=${PWD}/../${winball}
    ../configure --prefix=$prefix --enable-static --host=x86_64-w64-mingw32 $confopts
  fi

  make
  make install
  #make install-winball || true # not needed anymore in v1.7+
  if test x$docheck = x1; then
    make check
  fi
  utils/lstopo/lstopo-no-graphics -v
  winball=$(basename $(head config.log | sed -r -n -e 's/.*--prefix=([^ ]+).*/\1/p'))
  cd ..

  zip -r ../${winball}.zip ${winball}
  test -f ${winball}/lib/libhwloc.lib || false
  if test x$dotar = x1; then
    cd ..
  fi

fi

PATH=$oldPATH
