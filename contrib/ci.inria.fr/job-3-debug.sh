#!/bin/sh
#
# Copyright © 2012-2015 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# remove everything but the last 10 builds
ls | grep -v ^hwloc- | grep -v ^job- | xargs rm -rf || true
ls -td hwloc-* | tail -n +11 | xargs chmod u+w -R || true
ls -td hwloc-* | tail -n +11 | xargs rm -rf || true

# find the tarball, extract it
tarball=$(ls -tr hwloc-*.tar.gz | grep -v build.tar.gz | tail -1)
basename=$(basename $tarball .tar.gz)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball
rm $tarball
cd $basename

# ignore clock problems
touch configure

# build with plugins and debug
mkdir build-plugins-debug
cd build-plugins-debug
../configure --enable-plugins --enable-debug
make
make check
tests/hwloc/wrapper.sh utils/lstopo/lstopo-no-graphics
cd ..

# cleanup
rm -rf doc build-plugins-debug/doc
cd ..
tar cfz ${basename}.build.tar.gz $basename
rm -rf $basename
