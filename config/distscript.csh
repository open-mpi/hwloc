#! /bin/csh -f
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright © 2010 inria.  All rights reserved.
# Copyright © 2009-2013 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

set builddir="`pwd`"

set srcdir="$1"
cd "$srcdir"
set srcdir=`pwd`
cd "$builddir"

set distdir="$builddir/$2"
set HWLOC_VERSION="$3"

if ("$distdir" == "") then
    echo "Must supply relative distdir as argv[2] -- aborting"
    exit 1
elif ("$HWLOC_VERSION" == "") then
    echo "Must supply version as argv[1] -- aborting"
    exit 1
endif

#========================================================================

if ("$srcdir" != "$builddir") then
    set vpath=1
    set vpath_msg=yes
else
    set vpath=0
    set vpath_msg=no
endif

set start=`date`
cat <<EOF
 
Creating hwloc distribution
In directory: `pwd`
Srcdir: $srcdir
Builddir: $builddir
VPATH: $vpath_msg
Version: $HWLOC_VERSION
Started: $start
 
EOF

umask 022

if (! -d "$distdir") then
    echo "*** ERROR: dist dir does not exist"
    echo "*** ERROR:   $distdir"
    exit 1
endif

#
# VPATH builds only work if the srcdir has valid docs already built.
# If we're VPATH and the srcdir doesn't have valid docs, then fail.
#

if ($vpath == 1 && ! -d $srcdir/doc/doxygen-doc) then
    echo "*** This is a VPATH 'make dist', but the srcdir does not already"
    echo "*** have a doxygen-doc tree built.  hwloc's config/distscript.csh"
    echo "*** requores the docs to be built in the srcdir before executing"
    echo "*** 'make dist' in a VPATH build."
    exit 1
endif

#
# If we're not VPATH, force the generation of new doxygen documentation
#

if ($vpath == 0) then
    # Not VPATH
    echo "*** Making new doxygen documentation (doxygen-doc tree)"
    echo "*** Directory: srcdir: $srcdir, distdir: $distdir, pwd: `pwd`"
    cd doc
    # We're still in the src tree, so kill any previous doxygen-docs
    # tree and make a new one.
    chmod -R a=rwx doxygen-doc
    rm -rf doxygen-doc
    make
    if ($status != 0) then
        echo ERROR: generating doxygen docs failed
        echo ERROR: cannot continue
        exit 1
    endif

    # Make new README file
    echo "*** Making new README"
    make readme
    if ($status != 0) then
        echo ERROR: generating new README failed
        echo ERROR: cannot continue
        exit 1
    endif
else
    echo "*** This is a VPATH build; assuming that the doxygen docs and REAME"
    echo "*** are current in the srcdir (i.e., we'll just copy those)"
endif

echo "*** Copying doxygen-doc tree to dist..."
echo "*** Directory: srcdir: $srcdir, distdir: $distdir, pwd: `pwd`"
chmod -R a=rwx $distdir/doc/doxygen-doc
echo rm -rf $distdir/doc/doxygen-doc
rm -rf $distdir/doc/doxygen-doc
echo cp -rpf $srcdir/doc/doxygen-doc $distdir/doc
cp -rpf $srcdir/doc/doxygen-doc $distdir/doc

echo "*** Copying new README"
ls -lf $distdir/README
cp -pf $srcdir/README $distdir

#########################################################
# VERY IMPORTANT: Now go into the new distribution tree #
#########################################################
cd "$distdir"
echo "*** Now in distdir: $distdir"

#
# Remove all the latex source files from the distribution tree (the
# PDFs are still there; we're just removing the latex source because
# some of the filenames get really, really long...).
#

echo "*** Removing latex source from dist tree"
rm -rf doc/doxygen-doc/latex

#
# All done
#

cat <<EOF
*** hwloc version $HWLOC_VERSION distribution created
 
Started: $start
Ended:   `date`
 
EOF

