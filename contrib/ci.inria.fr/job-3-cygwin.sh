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

# extract the tarball
tarball="$1"
basename=$(basename $tarball .tar.gz)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball
rm $tarball
cd $basename

# ignore clock problems
touch configure

# build without plugins, with relative VPATH
mkdir build
cd build
../configure
make
make check
utils/lstopo/lstopo-no-graphics -v
cd ..

exit 0
