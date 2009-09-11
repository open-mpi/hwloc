# Copyright 2009 INRIA, Université Bordeaux 1
# Copyright © 2009 Cisco Systems, Inc.  All rights reserved.

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src utils tests include doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hwloc.pc

#
# Double check that we generated both the doxygen docs and a new copy
# of the top-level README file.
#

cannot-dist:
	@echo "ERROR: Did not build both of the doxygen docs and README."
	@echo "ERROR: This tarball is not complete!"
	@echo "ERROR: Cowardly refusing to complete successfully..."
	@exit 1

if !BUILD_DOXYGEN
dist-hook: cannot-dist
else
if !BUILD_README
dist-hook: cannot-dist
endif
endif

#
# Build the top-level README file
#

.PHONY: doc readme
doc readme:
	$(MAKE) -C doc readme
