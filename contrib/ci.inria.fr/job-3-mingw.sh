#!/bin/sh
#
# Copyright © 2012-2015 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

dotar=1
dokeeptar=0
doconf=1
dobuild32=1
dobuild64=1
docheck=1
doinstall=1
confopts=

while test $# -gt 0; do
  if test "$1" = "--no-tar"; then
    dotar=0
  else if test "$1" = "--keep-tar"; then
    dokeeptar=1
  else if test "$1" = "--no-conf"; then
    doconf=0
  else if test "$1" = "--debug"; then
    confopts="$confopts --enable-debug"
  else if test "$1" = "--no-32"; then
    dobuild32=0
  else if test "$1" = "--no-64"; then
    dobuild64=0
  else if test "$1" = "--no-check"; then
    docheck=0
  else if test "$1" = "--no-install"; then
    doinstall=0
  else if test "$1" = "--help" -o "$1" = "-h"; then
    echo "  --no-tar      Use current directory instead of latest hwloc-*.tar.gz"
    echo "  --keep-tar    Don't delete tarball after unpacking"
    echo "  --no-conf     Don't reconfigure tarball"
    echo "  --debug       Enable debug"
    echo "  --no-32       Don't build in 32bits"
    echo "  --no-64       Don't build in 64bits"
    echo "  --no-check    Don't run make check"
    echo "  --no-install  Don't install"
    echo "  --help        Show this help"
    exit 0
  fi fi fi fi fi fi fi fi fi
  shift
done

oldPATH=$PATH

if test x$dotar = x1; then
  # remove everything but the last 10 builds
  rm -rf $(ls | grep -v ^hwloc- | grep -v ^job-) || true
  chmod u+w -R $(ls -td hwloc-* | tail -n +11) || true
  rm -rf $(ls -td hwloc-* | tail -n +11) || true

  # find the tarball and extract it
  tarball=$(ls -tr hwloc-*.tar.gz | tail -1)
  basename=$(basename $tarball .tar.gz)
  version=$(echo $basename | cut -d- -f2)
  test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
  tar xfz $tarball
  if test x$dokeeptar = x0; then
    rm $tarball
  fi
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

  if test x$docheck = x1; then
    make check
  fi

  if test x$doinstall = x1; then
    make install
    #make install-winball || true # not needed anymore in v1.7+
    winball=$(basename $(head config.log | sed -r -n -e 's/.*--prefix=([^ ]+).*/\1/p'))
    cd ..

    zip -r ../${winball}.zip ${winball}
    test -f ${winball}/lib/libhwloc.lib || false
  else
    cd ..
  fi

  build32/utils/lstopo/lstopo-no-graphics -v

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

  if test x$docheck = x1; then
    make check
  fi

  if test x$doinstall = x1; then
    make install
    #make install-winball || true # not needed anymore in v1.7+
    winball=$(basename $(head config.log | sed -r -n -e 's/.*--prefix=([^ ]+).*/\1/p'))
    cd ..

    zip -r ../${winball}.zip ${winball}
    test -f ${winball}/lib/libhwloc.lib || false
  else
    cd ..
  fi

  build64/utils/lstopo/lstopo-no-graphics -v

  if test x$dotar = x1; then
    cd ..
  fi

fi

PATH=$oldPATH
