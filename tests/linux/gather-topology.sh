#!/bin/sh
#-*-sh-*-

# Copyright (C) 2009 INRIA
#

name="$1"; shift
[ -z "$name" ] && echo "Save name needed as an argument" && exit -1

destdir=`mktemp -d`

# Get all files from the given path (either a file or a directory)
# ignore errors since some files may be missing, and some may be
# Restricted to root (but we don't need them).
# Use cat so that we properly get proc/sys files even if their
# file length is wrong
savepath() {
  local dest="$1"
  local path="$2"
  find "$path" -type f 2>/dev/null | while read file ; do	\
    mkdir -p "$dest/"`dirname $file` ;		\
    cat "$file" > "$dest/$file" 2>/dev/null ;	\
  done
}

# Gather the following list of files and directories
cat << EOF | while read path ; do savepath "$destdir/$name" "$path" ; done
/sys/devices/system/cpu/
/sys/devices/system/node/
/sys/class/dmi/id/
/proc/cpuinfo
/proc/meminfo
/proc/stat
EOF

# Create the archive and keep the tree in /tmp for testing
( cd "$destdir/" && tar cfz "$name.tar.gz" "$name" )
mv "$destdir/$name.tar.gz" "./$name.tar.gz"
echo "Hierarchy gathered in ./$name.tar.gz and kept in $destdir/$name/"

exit 0
