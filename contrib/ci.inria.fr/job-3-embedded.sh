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
mv $tarball $basename # keep the tarball, we'll need it for the embedded test below
cd $basename

# ignore clock problems
touch configure

# embedded tests
(cd tests/hwloc/embedded && ./run-embedded-tests.sh ../../../$tarball)

exit 0
