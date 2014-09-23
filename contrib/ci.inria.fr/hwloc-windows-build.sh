#!/bin/sh

#
# Copyright © 20012-2014 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

oldPATH=$PATH

tarball=$(ls -tr hwloc-*.tar.gz | tail -1)
basename=$(basename $tarball .tar.gz)
version=$(echo $basename | cut -d- -f2)

test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball

mkdir ${basename}/build32
cd ${basename}/build32
winball=hwloc-win32-build-${version}
prefix=${PWD}/../${winball}
export PATH=/c/Builds:/c/Builds/mingw32/bin/:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
../configure --prefix=$prefix --enable-static --host=i686-w64-mingw32 CC_FOR_BUILD=x86_64-w64-mingw32-gcc
make
make install
#make install-winball || true # not needed anymore in v1.7+
make check
utils/lstopo/lstopo-no-graphics -v
cd ..
zip -r ../${winball}.zip ${winball}
test -f ${winball}/lib/libhwloc.lib || false
cd ..


mkdir ${basename}/build64
cd ${basename}/build64
winball=hwloc-win64-build-${version}
prefix=${PWD}/../${winball}
export PATH=/c/Builds:/c/Builds/mingw64/bin/:/c/Builds/mingw32/i686-w64-mingw32/lib/:"/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin":"/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE":$oldPATH
../configure --prefix=$prefix --enable-static --host=x86_64-w64-mingw32
make
make install
#make install-winball || true # not needed anymore in v1.7+
make check
utils/lstopo/lstopo-no-graphics -v
cd ..
zip -r ../${winball}.zip ${winball}
test -f ${winball}/lib/libhwloc.lib || false
cd ..

PATH=$oldPATH
