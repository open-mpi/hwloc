#!/bin/bash
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2021 Inria.  All rights reserved.
# See COPYING in top-level directory.
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
