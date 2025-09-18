#!/bin/bash
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2018-2025 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

version=$(./../../config/hwloc_get_version.sh ./../../VERSION --version)

cp ./../doxygen-config.cfg.in ./doxygen-config.cfg
sed -i 's/@top_srcdir@/..\/../g' ./doxygen-config.cfg
sed -i 's/@PACKAGE_VERSION@/'$version'/g' ./doxygen-config.cfg

echo "@INCLUDE = ./../doxygen.cfg" > readthedocs.io.cfg
echo "OUTPUT_DIRECTORY = $READTHEDOCS_OUTPUT/html/doxygen" >> readthedocs.io.cfg
echo "SHORT_NAMES = NO" >> readthedocs.io.cfg
echo "GENERATE_TAGFILE =" >> readthedocs.io.cfg
echo "GENERATE_MAN = NO" >> readthedocs.io.cfg
echo "GENERATE_LATEX = NO" >> readthedocs.io.cfg
echo "GENERATE_TREEVIEW = YES" >> readthedocs.io.cfg

echo "#### Contents of readthedocs.io.cfg:"
cat readthedocs.io.cfg
echo "####"

mkdir -p $READTHEDOCS_OUTPUT/html/doxygen
