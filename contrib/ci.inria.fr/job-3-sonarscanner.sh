#!/bin/bash
#
# Copyright © 2012-2021 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

echo "############################"
echo "Running on:"
uname -a
echo "Tarball: $3"
echo "############################"

set -e
set -x

git_repo_url="$1"
hwloc_branch="$2"
tarball="$3"

if test -z "$git_repo_url" || test -z "$hwloc_branch"; then
  echo "Need repo URL and branch name as arguments."
  exit 1
fi

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile

# check that this is either master or vX.Y
if test x$hwloc_branch != xmaster; then
  if test x$(echo "x${hwloc_branch}x" | sed -r -e 's/xv[0-9]+\.[0-9]+x//') != x; then
    echo "Sending non-master and non-stable branch output to 'tmp' branch on sonarqube server."
    hwloc_branch=tmp
  fi
fi

# check that the repo is the official one
if test x$git_repo_url != xhttps://github.com/open-mpi/hwloc.git; then
  if test x$FORCE_SONARQUBE = xtrue; then
    echo "Sending non-official repository output to 'tmp' branch on sonarqube server."
    hwloc_branch=tmp
  else
    echo "Ignoring non-official repository."
    exit 0
  fi
fi

# extract the tarball
basename=$(basename $tarball .tar.gz)
test -d $basename && chmod -R u+rwX $basename && rm -rf $basename
tar xfz $tarball
rm $tarball
cd $basename

# ignore clock problems
touch configure

# Configure, Make and Install
export CFLAGS="-O0 -g -fPIC --coverage -Wall -Wunused-parameter -Wundef -Wno-long-long -Wsign-compare -Wmissing-prototypes -Wstrict-prototypes -Wcomment -pedantic -fdiagnostics-show-option -fno-inline"
export LDFLAGS="--coverage"
./configure
make V=1 | tee hwloc-build.log
# Execute unitary tests (autotest)
make check

# Collect coverage data
find . -path '*/.libs/*.gcno' -exec rename 's@/.libs/@/@' {} \;
find . -path '*/.libs/*.gcda' -exec rename 's@/.libs/@/@' {} \;
lcov --directory . --capture --output-file hwloc.lcov
lcov_cobertura.py hwloc.lcov --output hwloc-coverage.xml

# Clang/Scan-build
make distclean
export CFLAGS="-Wall -std=gnu99"
unset LDFLAGS
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports ./configure | tee scan-build.log
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make | tee -a scan-build.log
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make check | tee -a scan-build.log

# Run cppcheck analysis
SOURCES_TO_ANALYZE="src tests utils"
SOURCES_TO_EXCLUDE="-itests/ports -isrc/topology-aix.c -isrc/topology-bgq.c -isrc/topology-cuda.c -isrc/topology-darwin.c -isrc/topology-freebsd.c -isrc/topology-gl.c -isrc/topology-hpux.c -isrc/topology-netbsd.c -isrc/topology-nvml.c -isrc/topology-opencl.c -isrc/topology-osf.c -isrc/topology-solaris-chiptype.c -isrc/topology-solaris.c -isrc/topology-windows.c"
CPPCHECK_INCLUDES="-Iinclude -Isrc -Iutils/lstopo -Iutils/hwloc"
CPPCHECK="cppcheck -v -f --language=c --platform=unix64 --enable=all --xml --xml-version=2 --suppress=purgedConfiguration --suppress=missingIncludeSystem ${CPPCHECK_INCLUDES}"
# Main cppcheck
DEFINITIONS="-Dhwloc_getpagesize=getpagesize -DMAP_ANONYMOUS=0x20 -Dhwloc_thread_t=pthread_t -DSYS_gettid=128"
${CPPCHECK} ${DEFINITIONS} ${SOURCES_TO_ANALYZE} ${SOURCES_TO_EXCLUDE} 2> hwloc-cppcheck.xml
# cppcheck on non-Linux ports
DEFINITIONS="-DR_THREAD=6 -DP_DEFAULT=0 -DR_L2CSDL=11 -DR_PCORESDL=12 -DR_REF1SDL=13"
${CPPCHECK} ${DEFINITIONS} src/topology-aix.c -Itests/ports/include/aix 2> hwloc-cppcheck-aix.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-bgq.c -Itests/ports/include/bgq 2> hwloc-cppcheck-bgq.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-darwin.c -Itests/ports/include/darwin 2> hwloc-cppcheck-darwin.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-freebsd.c -Itests/ports/include/freebsd 2> hwloc-cppcheck-freebsd.xml
DEFINITIONS="-DMAP_MEM_FIRST_TOUCH=2 -Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-hpux.c -Itests/ports/include/hpux 2> hwloc-cppcheck-hpux.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-osf.c -Itests/ports/include/osf 2> hwloc-cppcheck-osf.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} src/topology-netbsd.c -Itests/ports/include/netbsd 2> hwloc-cppcheck-netbsd.xml
DEFINITIONS="-DMADV_ACCESS_LWP=7"
${CPPCHECK} ${DEFINITIONS} src/topology-solaris.c src/topology-solaris-chiptype.c -Itests/ports/include/solaris 2> hwloc-cppcheck-solaris.xml
DEFINITIONS="-Dhwloc_thread_t=HANDLE"
${CPPCHECK} ${DEFINITIONS} src/topology-windows.c -Itests/ports/include/windows 2> hwloc-cppcheck-windows.xml
# cppcheck on GPU ports
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-cuda.c -Itests/ports/include/cuda 2> hwloc-cppcheck-cuda.xml
DEFINITIONS="-DNV_CTRL_PCI_DOMAIN=306"
${CPPCHECK} ${DEFINITIONS} src/topology-gl.c -Itests/ports/include/gl 2> hwloc-cppcheck-gl.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} src/topology-nvml.c -Itests/ports/include/nvml 2> hwloc-cppcheck-nvml.xml
DEFINITIONS="-DCL_DEVICE_BOARD_NAME_AMD=0x4038 -DCL_DEVICE_TOPOLOGY_AMD=0x4037"
${CPPCHECK} ${DEFINITIONS} src/topology-opencl.c -Itests/ports/include/opencl 2> hwloc-cppcheck-opencl.xm

CPPCHECK_XMLS=$(ls -m hwloc-cppcheck*.xml)

# Run rats analysis
rats -w 3 --xml ${SOURCES_TO_ANALYZE} > hwloc-rats.xml

RATS_XMLS=$(ls -m hwloc-rats*.xml)

# Run valgrind analysis
VALGRIND="libtool --mode=execute valgrind --leak-check=full --xml=yes"
$VALGRIND --xml-file=hwloc-valgrind-local.xml utils/lstopo/lstopo-no-graphics - >/dev/null
$VALGRIND --xml-file=hwloc-valgrind-xml1.xml utils/lstopo/lstopo-no-graphics -i tests/xml/32em64t-2n8c2t-pci-normalio.xml -.xml >/dev/null

VALGRIND_XMLS=$(ls -m hwloc-valgrind*.xml)

# Disable GCC attributes, sonar-scanner doesn't support it
sed -e '/#define HWLOC_HAVE_ATTRIBUTE/d' -i include/private/autogen/config.h

# Create the config for sonar-scanner
cat > sonar-project.properties << EOF
sonar.host.url=https://sonarqube.inria.fr/sonarqube
sonar.login=$(cat ~/.sonarqube-hwloc-token)
sonar.links.homepage=https://www.open-mpi.org/projects/hwloc/
sonar.links.ci=https://ci.inria.fr/hwloc/
sonar.links.scm=https://github.com/open-mpi/hwloc
sonar.links.issue=https://github.com/open-mpi/hwloc/issues
sonar.projectKey=tadaam:hwloc:github:$hwloc_branch
sonar.projectDescription=Hardware locality (hwloc)
# SED doesn't want us to define projectName
sonar.projectVersion=$hwloc_branch
sonar.scm.disabled=false
# sonar.scm.provider=git requires sonar-scanner to run inside a git clone
sonar.sourceEncoding=UTF-8
sonar.sources=src, tests, utils
sonar.exclusions=tests/ports
sonar.c.clangsa.reportPath=analyzer_reports/*/*.plist
sonar.c.errorRecoveryEnabled=true
sonar.c.compiler.parser=GCC
sonar.c.compiler.charset=UTF-8
sonar.c.compiler.regex=^(.*):(\\\d+):\\\d+: warning: (.*)\\\[(.*)\\\]$
sonar.c.compiler.reportPath=hwloc-build.log
sonar.c.coverage.reportPath=hwloc-coverage.xml
sonar.c.cppcheck.reportPath=${CPPCHECK_XMLS}
sonar.c.includeDirectories=$(echo | gcc -E -Wp,-v - 2>&1 | grep "^ " | tr '\n' ',')include,src,utils/lstopo,utils/hwloc
sonar.c.rats.reportPath=${RATS_XMLS}
sonar.c.valgrind.reportPath=${VALGRIND_XMLS}
# make sure only C matches our .c files
sonar.lang.patterns.c++=**/*.cpp,**/*.hpp
EOF

# Run the sonar-scanner analysis and submit to SonarQube server
sonar-scanner -X > sonar.log

exit 0
