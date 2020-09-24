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

# build android
tar zxvf ../android.tar.gz
cd contrib/android/AndroidApp
chmod +x gradlew
./gradlew --full-stacktrace build

# move apk in specific folder (alphabetically ordered after the main tarballs in the jenkins artifacts)
rm -rf ../../../../lstopo-android
mkdir ../../../../lstopo-android
mv lstopo/build/outputs/apk/debug/*.apk ../../../../lstopo-android

exit 0
