#!/bin/sh
file="`mktemp`"
if ! [ -d "$file" ]
then
	file=/tmp/topodistrib-$$
	old_umask=`umask`
	umask 0077
	touch "$file"
	umask $old_umask
fi
set -e
(
  ./topodistrib --synthetic "2 2 2" 2
  echo
  ./topodistrib --synthetic "2 2 2" 4
  echo
  ./topodistrib --synthetic "2 2 2" 8
  echo
  ./topodistrib --synthetic "2 2 2" 13
  echo
  ./topodistrib --synthetic "2 2 2" 16
  echo
  ./topodistrib --synthetic "3 3 3" 4
) > "$file"
diff -u $srcdir/test-topodistrib.output "$file"
rm -f "$file"
