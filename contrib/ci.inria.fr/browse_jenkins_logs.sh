#!/bin/bash

if test "x$1" = x -o "x$1" = "x-h" -o "x$1" = "x--help"; then
  echo "$0 <job> [<build>]"
  echo "  open all build logs from a job on ci.inria.fr jenkins."
  echo
  echo "  If \$HWLOC_JENKINS_BROWSER isn't set (for instance, to 'firefox'),"
  echo "  URLs are listed in the terminal"
  echo
  echo "  <job> may be 'basic/job/master' ('basic' multibranch pipeline on branch 'master')"
  echo "    or 'basic/view/change-requests/job/PR-302' ('basic' multibranch pipeline on pull request #302)"
  echo "    or 'bgoglin' (normal pipeline)"
  echo
  echo "  If the build number <build> isn't specified, 'lastBuild' is used."
  exit 1
fi

JOB="$1"
shift

BUILD="$1"
if test "x$BUILD" = x; then
  BUILD=lastBuild
fi

URL=https://ci.inria.fr/hwloc/job/$JOB/$BUILD/flowGraphTable/
echo "Reading $URL"

LIST=$(wget -O - $URL \
        | sed -e 's@</tr>@</tr>\n@g' \
        | egrep '(Shell|Windows Batch) Script' \
        | egrep 'job-[0-9]-' \
        | sed -r -e 's#.*href="(/hwloc/[^"]+)".*#https://ci.inria.fr\1/\?consoleFull#')

echo
if test "x$HWLOC_JENKINS_BROWSER" = x; then
  echo "URLs are:"
  echo $LIST
else
  echo "Opening URLs with HWLOC_JENKINS_BROWSER=$HWLOC_JENKINS_BROWSER ..."
  $HWLOC_JENKINS_BROWSER $LIST
fi
