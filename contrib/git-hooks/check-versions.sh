#!/bin/bash
#
# Copyright © 2018-2020 Inria.  All rights reserved.
# $COPYRIGHT$
#

function die() {
  echo "$@"
  exit 1
}

android_gradle="./contrib/android/AndroidApp/lstopo/build.gradle"
android_config_h="./contrib/android/include/hwloc/autogen/config.h"
windows_config_h="./contrib/windows/hwloc_config.h"
vcxproj_file="./contrib/windows/libhwloc.vcxproj"
version_file="./VERSION"

# get all individual version components
android_version=$(grep -w HWLOC_VERSION $android_config_h | grep -oP '".+"' | tr -d \")
android_major=$(grep -w HWLOC_VERSION_MAJOR $android_config_h | grep -oP '[0-9]+')
android_minor=$(grep -w HWLOC_VERSION_MINOR $android_config_h | grep -oP '[0-9]+')
android_release=$(grep -w HWLOC_VERSION_RELEASE $android_config_h | grep -oP '[0-9]+')
android_greek=$(grep -w HWLOC_VERSION_GREEK $android_config_h | grep -oP '".*"' | tr -d \")
android_gradle_version=$(grep -w versionName $android_gradle | grep -oP '".*"' | tr -d \" | grep -oP '.+?(?=[-~])')
windows_major=$(grep -w HWLOC_VERSION_MAJOR $windows_config_h | grep -oP '[0-9]+')
windows_minor=$(grep -w HWLOC_VERSION_MINOR $windows_config_h | grep -oP '[0-9]+')
windows_release=$(grep -w HWLOC_VERSION_RELEASE $windows_config_h | grep -oP '[0-9]+')
windows_greek=$(grep -w HWLOC_VERSION_GREEK $windows_config_h | grep -oP '".*"' | tr -d \")
official_major=$(grep ^major= $version_file  | cut -d= -f2)
official_minor=$(grep ^minor= $version_file  | cut -d= -f2)
official_release=$(grep ^release= $version_file  | cut -d= -f2)


### WINDOWS CHECK ###
if [ -z "$windows_major" -o -z "$windows_minor" -o -z "$windows_release" ]; then
	# greek is likely empty on purpose, ignore it
	die "ERROR in $windows_config_h: Failed to get Windows-specific HWLOC_VERSION_MAJOR/MINOR/RELEASE"
fi

# check that the version string matches
windows_version=$(grep -w HWLOC_VERSION $windows_config_h | grep -oP '".+"' | tr -d \")
expected_windows_version="$windows_major.$windows_minor.$windows_release$windows_greek"
if [ "$windows_version" != "$expected_windows_version" ]; then
	die "ERROR in $windows_config_h: Windows-specific HWLOC_VERSION \"$windows_version\" doesn't match HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK components \"$expected_windows_version\""
fi

# check that it matchs the official version, without a GREEK
if [ -z "$official_major" -o -z "$official_minor" -o -z "$official_release" ]; then
	die "ERROR in $windows_config_h: Failed to get official HWLOC_VERSION_MAJOR/MINOR/RELEASE"
fi
official_version_nogreek="$official_major.$official_minor.$official_release"
if [ "$official_version_nogreek" != "$windows_version" ]; then
	die "ERROR in $windows_config_h: Windows-specific HWLOC_VERSION \"$windows_version\" doesn't match the official \"$official_version_nogreek\" without GREEK"
fi

# get the windows soname
if [ `grep '<TargetName>' $vcxproj_file | uniq -c | wc -l` != 1 ]; then
	die "ERROR in $vcxproj_file: Couldn't find a single value for <TargetName> lines"
fi
windows_lib_soname=$(grep -m1 '<TargetName>' $vcxproj_file | grep -oP '\d+')
if [ -z "$windows_lib_soname" ]; then
	die "ERROR in $vcxproj_file: Failed to get the Windows-specific soname"
fi

# get the official soname
official_lib_version=$(grep -w "libhwloc_so_version" $version_file | grep -oP '\d+:\d+:\d+')
if [ -z "$official_lib_version" ]; then
	die "ERROR in $version_file: Failed to get the official lib version"
fi

# bashisms to extract the soname from the version
IFS=':' arr=(${official_lib_version})
declare -i official_lib_soname=${arr[0]}-${arr[2]}

# check that sonames match only if on a release branch
if [ "$official_lib_version" != "0:0:0" ] ; then
	if [ "$windows_lib_soname" != "$official_lib_soname" ]; then
		die "ERROR in $vcxproj_file: Windows-specific lib soname $windows_lib_soname differs from official $official_lib_soname (from \"$official_lib_version\")"
	fi
fi

### ANDROID CHECK ###
if [ -z "$android_major" -o -z "$android_minor" -o -z "$android_release" ]; then
	# greek is likely empty on purpose, ignore it
	die "ERROR in $android_config_h: Failed to get Android-specific HWLOC_VERSION_MAJOR/MINOR/RELEASE"
fi

# check that the version string matches
expected_android_version="$android_major.$android_minor.$android_release$android_greek"
if [ "$android_version" != "$expected_android_version" ]; then
	die "ERROR in $android_config_h: Android-specific HWLOC_VERSION \"$android_version\" doesn't match HWLOC_VERSION_MAJOR/MINOR/RELEASE/GREEK components \"$expected_android_version\""
fi

# check android config.h version
if [ "$official_version_nogreek" != "$android_version" ]; then
	die "ERROR in $android_config_h: Android-specific HWLOC_VERSION \"$android_version\" doesn't match \"$official_version_nogreek\" without GREEK"
fi

# check android gradle version
if [ "$official_version_nogreek" != "$android_gradle_version" ]; then
	die "ERROR in $android_gradle: Android gradle HWLOC_VERSION \"$android_gradle_version\" doesn't match \"$official_version_nogreek\" without GREEK"
fi
