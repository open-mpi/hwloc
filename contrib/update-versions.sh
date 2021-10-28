#!/bin/bash
#
# Copyright © 2021 Inria.  All rights reserved.
# $COPYRIGHT$
#

#
# Update windows/cmake/android versions from VERSION
#

./contrib/windows/check-versions.sh --update
echo
echo
./contrib/android/check-versions.sh --update
echo
echo
./contrib/windows-cmake/check-versions.sh --update
