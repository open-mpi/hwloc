#!/bin/sh
#
# Copyright © 2012-2020 Inria.  All rights reserved.
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

# build without plugins, with relative VPATH
mkdir build
cd build
../configure $HWLOC_CI_JOB1CHECK_CONFOPTS
make
test x$NO_CHECK = xtrue || make check
utils/lstopo/lstopo-no-graphics -v
cd ..

# build with plugins, with absolute VPATH
mkdir build-plugins
cd build-plugins
$PWD/../configure --enable-plugins $HWLOC_CI_JOB1CHECK_CONFOPTS
make
test x$NO_CHECK = xtrue || make check
tests/hwloc/wrapper.sh utils/lstopo/lstopo-no-graphics -v
tests/hwloc/wrapper.sh utils/hwloc/hwloc-info --support
cd ..

# check renaming
test x$NO_CHECK = xtrue || (cd build/tests/hwloc/rename && make check)

exit 0
