#!/bin/sh
#
# Copyright © 2012-2018 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

echo "############################"
echo "Running on:"
uname -a
echo "Tarball: $1"
echo "############################"

set -e
set -x

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# extract the tarball
tarball="$1"
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

exit 0
