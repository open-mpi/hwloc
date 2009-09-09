# Copyright 2009 INRIA, Université Bordeaux 1

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src utils tests include doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hwloc.pc

.PHONY: doc doc-upload
doc doc-upload:
	$(MAKE) -C doc $@
