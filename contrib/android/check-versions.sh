#!/bin/bash
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2018-2023 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

function die() {
  echo "$@"
  exit 1
}

if test "x$1" = "x-h" -o "x$1" = "x--help"; then
  echo "$0 [--quiet] [--update] [git root directory]"
  exit 0
fi

echo=echo
if test "x$1" = "x--quiet"; then
  echo=true
  shift
fi

if test "x$1" = "x--update"; then
  update=1
  shift
fi

rootdir=.
if test "x$1" != x; then
  rootdir="$1"
fi

android_gradle="$rootdir/contrib/android/AndroidApp/lstopo/build.gradle"
android_config_h="$rootdir/contrib/android/include/hwloc/autogen/config.h"
version_file="$rootdir/VERSION"

if ! test -f "$version_file"; then
  die "Couldn't find $version_file"
fi

### EXTRACT MAIN VERSION ###
$echo "Looking for official version $version_file ..."
official_major=$(grep ^major= $version_file  | cut -d= -f2)
official_minor=$(grep ^minor= $version_file  | cut -d= -f2)
official_release=$(grep ^release= $version_file  | cut -d= -f2)
official_greek=$(grep ^greek= $version_file | cut -d= -f2)
if [ -z "$official_major" -o -z "$official_minor" -o -z "$official_release" ]; then
	die "ERROR in $version_file: Failed to get official HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK"
fi
$echo "  Found major=$official_major minor=$official_minor release=$official_release greek=$official_greek"
official_version="$official_major.$official_minor.$official_release$official_greek"

$echo

### ANDROID UPDATE? ###
if test "x$update" = "x1"; then
  $echo "Updating Android VERSION in $android_config_h ..."
  sed -r -e '/define HWLOC_VERSION "/s/"[0-9a-zA-Z\.-]+"/"'$official_version'"/' -i "$android_config_h"
  sed -r -e '/define HWLOC_VERSION_MAJOR /s/[0-9]+/'$official_major'/' -i "$android_config_h"
  sed -r -e '/define HWLOC_VERSION_MINOR /s/[0-9]+/'$official_minor'/' -i "$android_config_h"
  sed -r -e '/define HWLOC_VERSION_RELEASE /s/[0-9]+/'$official_release'/' -i "$android_config_h"
  sed -r -e '/define HWLOC_VERSION_GREEK "/s/"[0-9a-zA-Z\.-]*"/"'$official_greek'"/' -i "$android_config_h"
  $echo "Updating Android VERSION in $android_gradle ..."
  # replace oldversion-X-Y with newversion-1-Y since a version bump usually puts back snapshot number to 1
  suffix=$(grep -w versionName $android_gradle | grep -oP '".*"' | tr -d \" | grep -oP -- '-[0-9\.]+$')
  sed -r -e '/versionName "/s/"[0-9a-zA-Z\.-]+"/"'$official_version'-1'$suffix'"/' -i $android_gradle
  $echo
fi

### ANDROID CHECKS ###
$echo "Looking for Android-specific version in $android_config_h ..."
android_version=$(grep -w HWLOC_VERSION $android_config_h | grep -oP '".+"' | tr -d \")
android_major=$(grep -w HWLOC_VERSION_MAJOR $android_config_h | grep -oP '[0-9]+')
android_minor=$(grep -w HWLOC_VERSION_MINOR $android_config_h | grep -oP '[0-9]+')
android_release=$(grep -w HWLOC_VERSION_RELEASE $android_config_h | grep -oP '[0-9]+')
android_greek=$(grep -w HWLOC_VERSION_GREEK $android_config_h | grep -oP '".*"' | tr -d \")
if [ -z "$android_major" -o -z "$android_minor" -o -z "$android_release" ]; then
	# greek is likely empty on purpose, ignore it
	die "ERROR in $android_config_h: Failed to get Android-specific HWLOC_VERSION_MAJOR/MINOR/RELEASE"
fi
$echo "  Found Android-specific version=$android_version major=$android_major minor=$android_minor release=$android_release greek=$android_greek"

# check that the version string matches
expected_android_version="$android_major.$android_minor.$android_release$android_greek"
if [ "$android_version" != "$expected_android_version" ]; then
	die "ERROR in $android_config_h: Android-specific HWLOC_VERSION \"$android_version\" doesn't match HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK components \"$expected_android_version\""
fi
$echo "  Android-specific HWLOC_VERSION \"$android_version\" matches HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK components"

# check android config.h version
if [ "$official_version" != "$android_version" ]; then
	die "ERROR in $android_config_h: Android-specific HWLOC_VERSION \"$android_version\" doesn't match \"$official_version\""
fi
$echo "  Android-specific version \"$android_version\" matches official version"

$echo

# check android gradle version
$echo "Looking for Android-specific version in $android_gradle ..."
android_gradle_version=$(grep -w versionName $android_gradle | grep -oP '".*"' | tr -d \" | grep -oP '[0-9a-z\.]+(?=-.*-.*)')
if [ "$official_version" != "$android_gradle_version" ]; then
	die "ERROR in $android_gradle: Android gradle HWLOC_VERSION \"$android_gradle_version\" doesn't match \"$official_version\""
fi
$echo "  Android-specific gradle version \"$android_gradle_version\" matches official version"
