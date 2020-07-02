#!/bin/bash
#
# Copyright © 2012-2020 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

echo "############################"
echo "Running on:"
uname -a
echo "Tarball: $3"
echo "############################"

set -e
set -x

git_repo_url="$1"
hwloc_branch="$2"
tarball="$3"

if test -z "$git_repo_url" || test -z "$hwloc_branch"; then
  echo "Need repo URL and branch name as arguments."
  exit 1
fi

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# check that the repo is the official one
# check that this is master
if test x$hwloc_branch != xmaster -o x$git_repo_url != xhttps://github.com/open-mpi/hwloc.git; then
  if test x$FORCE_COVERITY = xtrue; then
    echo "Forcing coverity on non-master-branch or non-official repository."
  else
    echo "Ignoring non-master-branch or non-official repository."
    exit 0
  fi
fi

# extract the tarball
basename=$(basename $tarball .tar.gz)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball
rm $tarball
cd $basename

# ignore clock problems
touch configure

# configure things
EMAIL=brice.goglin@labri.fr
VERSION=$basename
COVDIR=cov-int
COVBALL=myproject.tgz

# run
./configure --enable-plugins
cov-build --dir ${COVDIR} make all
test x$NO_CHECK = xtrue || cov-build --dir ${COVDIR} make check
tar cfvz ${COVBALL} ${COVDIR}
curl --form file=@${COVBALL} \
     --form "token=<${COVERITY_TOKEN_FILE}" \
     --form email=${EMAIL} \
     --form version=${VERSION} \
     --form description=manual \
     https://scan.coverity.com/builds?project=hwloc

exit 0
