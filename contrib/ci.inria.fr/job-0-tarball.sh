#!/bin/sh
#
# Copyright © 2012-2018 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

echo "############################"
echo "Running on:"
uname -a
echo "############################"

set -e
set -x

branch="$1"
if test -z "$branch"; then
  echo "Need branch name as argument."
  exit 1
fi

echo "Got GIT branch name $branch"

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# keep branch-name before the first - (e.g. v2.0-beta becomes v2.0)
# and look for the corresponding autotools
basebranch=$( echo $branch | sed -r -e 's@^.*/([^/]+)$@\1@' -e 's/-.*//' )
if test -d $HOME/local/hwloc-$basebranch ; then
  export PATH=$HOME/local/hwloc-${basebranch}/bin:$PATH
  echo using specific $HOME/local/hwloc-$basebranch
else
  export PATH=$HOME/local/hwloc-master/bin:$PATH
  echo using generic $HOME/local/hwloc-master
fi

# remove previous artifacts so that they don't exported again by this build
rm -f hwloc-*.tar.gz hwloc-*.tar.bz2 || true

# display autotools versions
automake --version | head -1
libtool --version | head -1
autoconf --version | head -1

# better version string
snapshot=$branch-`date +%Y%m%d.%H%M`.git`git show --format=format:%h -s`
sed	-e 's/^snapshot_version=.*/snapshot_version='$snapshot/ \
	-e 's/^snapshot=.*/snapshot=1/' \
	-i VERSION

# let's go
./autogen.sh
./configure
make
make distcheck

# this test requires bash and grep -P, only run it in the main job
make check -C contrib/windows

exit 0
