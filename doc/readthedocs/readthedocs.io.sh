#!/bin/bash
#
# Copyright © 2018-2021 Inria.  All rights reserved.
# $COPYRIGHT$
#

version=$(./../../config/hwloc_get_version.sh ./../../VERSION --version)

cp ./../doxygen-config.cfg.in ./doxygen-config.cfg
sed -i 's/@top_srcdir@/..\/../g' ./doxygen-config.cfg
sed -i 's/@PACKAGE_VERSION@/'$version'/g' ./doxygen-config.cfg


echo "@INCLUDE = ./../doxygen.cfg" > readthedocs.io.cfg
echo "OUTPUT_DIRECTORY = ./build" >> readthedocs.io.cfg
echo "SHORT_NAMES = NO" >> readthedocs.io.cfg
echo "GENERATE_TAGFILE =" >> readthedocs.io.cfg
echo "GENERATE_MAN = NO" >> readthedocs.io.cfg
echo "GENERATE_LATEX = NO" >> readthedocs.io.cfg
echo "GENERATE_TREEVIEW = YES" >> readthedocs.io.cfg
