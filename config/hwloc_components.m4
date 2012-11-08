# Copyright © 2012 Inria.  All rights reserved.
# See COPYING in top-level directory.


# HWLOC_LIST_STATIC_COMPONENTS
#
# Append to file $1 an array of components by listing component names in $2.
#
# $1 = filename
# $2 = list of component names
#
AC_DEFUN([HWLOC_LIST_STATIC_COMPONENTS], [
cat <<EOF >>[$1]
static const struct hwloc_component * hwloc_static_components[[]] = {
EOF
for comp in [$2]; do
  echo "  &hwloc_${comp}_component," >>[$1]
done
cat <<EOF >>[$1]
  NULL
};
EOF
])
