:

srcdir=$(dirname "$0")
if test "x$srcdir" != x; then
   # in case we ever autogen on a platform without dirname
  cd $srcdir
fi

autoreconf ${autoreconf_args:-"-ivf"}
