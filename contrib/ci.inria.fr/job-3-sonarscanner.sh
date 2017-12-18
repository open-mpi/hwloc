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
./configure --enable-plugins
make V=1 | tee hwloc-build.log
# Execute unitary tests (autotest)
test x$NO_CHECK = xtrue || make check

# Collect coverage data
find . -path '*/.libs/*.gcno' -exec rename 's@/.libs/@/@' {} \;
find . -path '*/.libs/*.gcda' -exec rename 's@/.libs/@/@' {} \;
lcov --directory . --capture --output-file hwloc.lcov
lcov_cobertura.py hwloc.lcov --output hwloc-coverage.xml

# Clang/Scan-build
make distclean
export CFLAGS="-Wall -std=gnu99"
unset LDFLAGS
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports ./configure --enable-plugins | tee scan-build.log
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make | tee -a scan-build.log
test x$NO_CHECK = xtrue || scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make check | tee -a scan-build.log

# Run cppcheck analysis
SOURCES_TO_ANALYZE="hwloc tests utils"
SOURCES_TO_EXCLUDE="-itests/hwloc/ports -ihwloc/topology-aix.c -ihwloc/topology-bgq.c -ihwloc/topology-darwin.c -ihwloc/topology-freebsd.c -ihwloc/topology-hpux.c -ihwloc/topology-netbsd.c -ihwloc/topology-solaris.c -ihwloc/topology-solaris-chiptype.c -ihwloc/topology-windows.c -ihwloc/topology-cuda.c -ihwloc/topology-gl.c -ihwloc/topology-nvml.c -ihwloc/topology-rsmi.c -ihwloc/topology-levelzero.c -ihwloc/topology-opencl.c -iutils/lstopo/lstopo-windows.c -iutils/lstopo/lstopo-android.c"
CPPCHECK_INCLUDES="-Iinclude -Ihwloc -Iutils/lstopo -Iutils/hwloc"
CPPCHECK="cppcheck -v -f --language=c --platform=unix64 --enable=all --xml --xml-version=2 --suppress=purgedConfiguration --suppress=missingIncludeSystem ${CPPCHECK_INCLUDES}"
# Main cppcheck
DEFINITIONS="-Dhwloc_getpagesize=getpagesize -DMAP_ANONYMOUS=0x20 -Dhwloc_thread_t=pthread_t -DSYS_gettid=128"
DEFINITIONS="${DEFINITIONS} -UHASH_BLOOM -UHASH_EMIT_KEYS -UHASH_FUNCTION"
${CPPCHECK} ${DEFINITIONS} ${SOURCES_TO_ANALYZE} ${SOURCES_TO_EXCLUDE} 2> hwloc-cppcheck.xml
# cppcheck on non-Linux ports
DEFINITIONS="-DR_THREAD=6 -DP_DEFAULT=0 -DR_L2CSDL=11 -DR_PCORESDL=12 -DR_REF1SDL=13"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-aix.c -Itests/hwloc/ports/include/aix 2> hwloc-cppcheck-aix.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-bgq.c -Itests/hwloc/ports/include/bgq 2> hwloc-cppcheck-bgq.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-darwin.c -Itests/hwloc/ports/include/darwin 2> hwloc-cppcheck-darwin.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t -DCPU_WHICH_DOMAIN=6"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-freebsd.c -Itests/hwloc/ports/include/freebsd 2> hwloc-cppcheck-freebsd.xml
DEFINITIONS="-DMAP_MEM_FIRST_TOUCH=2 -Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-hpux.c -Itests/hwloc/ports/include/hpux 2> hwloc-cppcheck-hpux.xml
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-netbsd.c -Itests/hwloc/ports/include/netbsd 2> hwloc-cppcheck-netbsd.xml
DEFINITIONS="-DMADV_ACCESS_LWP=7"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-solaris.c hwloc/topology-solaris-chiptype.c -Itests/hwloc/ports/include/solaris 2> hwloc-cppcheck-solaris.xml
DEFINITIONS="-Dhwloc_thread_t=HANDLE"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-windows.c -Itests/hwloc/ports/include/windows 2> hwloc-cppcheck-windows.xml
# cppcheck on GPU ports
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-cuda.c -Itests/hwloc/ports/include/cuda 2> hwloc-cppcheck-cuda.xml
DEFINITIONS="-DNV_CTRL_PCI_DOMAIN=306"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-gl.c -Itests/hwloc/ports/include/gl 2> hwloc-cppcheck-gl.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-nvml.c -Itests/hwloc/ports/include/nvml 2> hwloc-cppcheck-nvml.xml
DEFINITIONS="-DCL_DEVICE_BOARD_NAME_AMD=0x4038 -DCL_DEVICE_TOPOLOGY_AMD=0x4037"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-opencl.c -Itests/hwloc/ports/include/opencl 2> hwloc-cppcheck-opencl.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-rsmi.c -Itests/hwloc/ports/include/rsmi 2> hwloc-cppcheck-rsmi.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} hwloc/topology-levelzero.c -Itests/hwloc/ports/include/levelzero 2> hwloc-cppcheck-levelzero.xml
# cppcheck on non-Linux lstopo
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} utils/lstopo/lstopo-windows.c -Itests/hwloc/ports/include/windows 2> hwloc-cppcheck-lstopo-windows.xml
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} utils/lstopo/lstopo-android.c 2> hwloc-cppcheck-lstopo-android.xml

CPPCHECK_XMLS=$(ls -m hwloc-cppcheck*.xml)

# Run rats analysis
rats -w 3 --xml ${SOURCES_TO_ANALYZE} > hwloc-rats.xml

RATS_XMLS=$(ls -m hwloc-rats*.xml)

# Run valgrind analysis
VALGRIND="libtool --mode=execute valgrind --leak-check=full --xml=yes"
$VALGRIND --xml-file=hwloc-valgrind-local.xml utils/lstopo/lstopo-no-graphics - >/dev/null
$VALGRIND --xml-file=hwloc-valgrind-xml1.xml utils/lstopo/lstopo-no-graphics -i tests/hwloc/xml/32em64t-2n8c2t-pci-normalio.xml -.xml >/dev/null

VALGRIND_XMLS=$(ls -m hwloc-valgrind*.xml)

# Disable GCC attributes, sonar-scanner doesn't support it
sed -e '/#define HWLOC_HAVE_ATTRIBUTE/d' -i include/private/autogen/config.h

# Create the config for sonar-scanner
# The text between '<< EOF' and 'EOF' doesn't appear in the xtrace output (set -x)
# hence the contents ~/.sonarqube-hwloc-token doesn't appear in the public logs.
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
sonar.sources=hwloc, tests, utils
sonar.exclusions=tests/hwloc/ports/**,utils/netloc/draw/**,tests/hwloc/xml/*.xml
sonar.c.clangsa.reportPath=analyzer_reports/*/*.plist
sonar.c.errorRecoveryEnabled=true
sonar.c.compiler.parser=GCC
sonar.c.compiler.charset=UTF-8
sonar.c.compiler.regex=^(.*):(\\\d+):\\\d+: warning: (.*)\\\[(.*)\\\]$
sonar.c.compiler.reportPath=hwloc-build.log
sonar.c.coverage.reportPath=hwloc-coverage.xml
sonar.c.cppcheck.reportPath=${CPPCHECK_XMLS}
sonar.c.includeDirectories=$(echo | gcc -E -Wp,-v - 2>&1 | grep "^ " | tr '\n' ',')include,hwloc,utils/lstopo,utils/hwloc
sonar.c.rats.reportPath=${RATS_XMLS}
sonar.c.valgrind.reportPath=${VALGRIND_XMLS}
# make sure only C matches our .c files
sonar.lang.patterns.c++=**/*.cpp,**/*.hpp
EOF

# Log the sonar-scanner version in case of problem
sonar-scanner --version

# Run the sonar-scanner analysis and submit to SonarQube server
sonar-scanner -X > sonar.log

exit 0
