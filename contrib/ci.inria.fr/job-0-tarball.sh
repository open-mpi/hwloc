#!/bin/sh
#
# Copyright © 2012-2018 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# jenkins multibranch pipelines set BRANCH_NAME
echo "Trying to get GIT branch name from BRANCH_NAME ..."
branch="$BRANCH_NAME"
if test -z "$branch"; then
  # old jenkins non-pipeline jobs set GIt_BRANCH
  echo "Try falling back to GIT_BRANCH ..."
  branch="$GIT_BRANCH"
  if test -z "$branch"; then
    # other jobs must force git local branch name to match remote branch name
    echo "Fallback to the output of git branch | cut -c3- ..."
    branch=$(git branch | cut -c3-)
  fi
fi
echo "Got GIT branch name $branch"

# keep branch-name before the first - (e.g. v2.0-beta becomes v2.0)
# and look for the corresponding autotools
branch=$( echo $branch | sed -r -e 's@^.*/([^/]+)$@\1@' -e 's/-.*//' )
if test -d $HOME/local/hwloc-$branch ; then
  export PATH=$HOME/local/hwloc-${branch}/bin:$PATH
  echo using specific $HOME/local/hwloc-$branch
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
