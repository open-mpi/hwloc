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

# build without plugins
mkdir build
cd build
../configure
make
make check
utils/lstopo/lstopo-no-graphics -v
cd ..

# build with plugins
mkdir build-plugins
cd build-plugins
../configure --enable-plugins
make
make check
tests/wrapper.sh utils/lstopo/lstopo-no-graphics -v
cd ..

# check renaming
(cd build/tests/rename && make check)

# cleanup
rm -rf doc build/doc build-plugins/doc
cd ..
tar cfz ${basename}.build.tar.gz $basename
rm -rf $basename
