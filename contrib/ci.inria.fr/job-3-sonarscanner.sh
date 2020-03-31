#!/bin/bash
#
# Copyright © 2012-2020 Inria.  All rights reserved.
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
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports ./configure --enable-plugins | tee scan-build.log
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make | tee -a scan-build.log
scan-build -plist --intercept-first --analyze-headers -o analyzer_reports make check | tee -a scan-build.log

# Run cppcheck analysis
SOURCES_TO_ANALYZE="hwloc netloc tests utils"
SOURCES_TO_EXCLUDE="-itests/hwloc/ports -ihwloc/topology-aix.c -ihwloc/topology-bgq.c -ihwloc/topology-darwin.c -ihwloc/topology-freebsd.c -ihwloc/topology-hpux.c -ihwloc/topology-netbsd.c -ihwloc/topology-solaris.c -ihwloc/topology-solaris-chiptype.c -ihwloc/topology-windows.c -ihwloc/topology-cuda.c -ihwloc/topology-gl.c -ihwloc/topology-nvml.c -ihwloc/topology-opencl.c -iutils/lstopo/lstopo-windows.c"
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
DEFINITIONS="-Dhwloc_thread_t=pthread_t"
${CPPCHECK} ${DEFINITIONS} hwloc/topology-freebsd.c -Itests/hwloc/ports/include/freebsd 2> hwloc-cppcheck-freebsd.xml
DEFINITIONS="-DMAP_MEM_FIRST_TOUCH=2 -Dhwloc_thread_t=pthread_t -DCPU_WHICH_DOMAIN=6"
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
# cppcheck on non-Linux lstopo
DEFINITIONS=
${CPPCHECK} ${DEFINITIONS} utils/lstopo/lstopo-windows.c -Itests/hwloc/ports/include/windows 2> hwloc-cppcheck-lstopo-windows.xml

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
sonar.language=c
sonar.sources=hwloc, netloc, tests, utils
sonar.exclusions=tests/hwloc/ports
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
sonar.issue.ignore.multicriteria=c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15,c16,c17,c18,c19,c20,r1,r2,r3,r4,r5,r6,r7
# Sharing some naming conventions is a key point to make it possible for a team to efficiently collaborate. This rule allows to check that all class names match a provided regular expression.
sonar.issue.ignore.multicriteria.c1.ruleKey=c:ClassName
sonar.issue.ignore.multicriteria.c1.resourceKey=**/*
# The file is too complex (392 while maximum allowed is set to 200).
sonar.issue.ignore.multicriteria.c2.ruleKey=c:FileComplexity
sonar.issue.ignore.multicriteria.c2.resourceKey=**/*
# Add or update the header of this file.
sonar.issue.ignore.multicriteria.c3.ruleKey=c:FileHeader
sonar.issue.ignore.multicriteria.c3.resourceKey=**/*
# Rename this file to match this regular expression: "(([a-z_][a-z0-9_]*)|([A-Z][a-zA-Z0-9]+))$".
sonar.issue.ignore.multicriteria.c4.ruleKey=c:FileName
sonar.issue.ignore.multicriteria.c4.resourceKey=**/*
# The Cognitive Complexity of this function is 17 which is greater than 15 authorized.
sonar.issue.ignore.multicriteria.c5.ruleKey=c:FunctionCognitiveComplexity
sonar.issue.ignore.multicriteria.c5.resourceKey=**/*
# The Cyclomatic Complexity of this function is 30 which is greater than 10 authorized.
sonar.issue.ignore.multicriteria.c6.ruleKey=c:FunctionComplexity
sonar.issue.ignore.multicriteria.c6.resourceKey=**/*
# Rename function "hwloc_get_proc_last_cpu_location" to match the regular expression ^[a-z_][a-z0-9_]{2,30}$.
sonar.issue.ignore.multicriteria.c7.ruleKey=c:FunctionName
sonar.issue.ignore.multicriteria.c7.resourceKey=**/*
# 197 more comment lines need to be written to reach the minimum threshold of 25.0% comment density.
# BUG: this rule doesn't work, either with no prefix or "c" or "common-c"
sonar.issue.ignore.multicriteria.c8.ruleKey=common-c:InsufficientCommentDensity
sonar.issue.ignore.multicriteria.c8.resourceKey=**/*
# 3 more lines of code need to be covered by tests to reach the minimum threshold of 65.0% lines coverage.
# BUG: this rule doesn't work, either with no prefix or "c" or "common-c"
sonar.issue.ignore.multicriteria.c9.ruleKey=common-c:InsufficientLineCoverage
sonar.issue.ignore.multicriteria.c9.resourceKey=**/*
# Extract this magic number '3' into a constant, variable declaration or an enum.
sonar.issue.ignore.multicriteria.c10.ruleKey=c:MagicNumber
sonar.issue.ignore.multicriteria.c10.resourceKey=**/*
# Missing curly brace.
sonar.issue.ignore.multicriteria.c11.ruleKey=c:MissingCurlyBraces
sonar.issue.ignore.multicriteria.c11.resourceKey=**/*
# Refactor this code to not nest more than 3 if/switch/try/for/while/do statements.
sonar.issue.ignore.multicriteria.c12.ruleKey=c:NestedStatements
sonar.issue.ignore.multicriteria.c12.resourceKey=**/*
# Define a constant instead of duplicating this literal "linuxpci" 2 times.
sonar.issue.ignore.multicriteria.c13.ruleKey=c:StringLiteralDuplicated
sonar.issue.ignore.multicriteria.c13.resourceKey=**/*
# Replace all tab characters in this file by sequences of white-spaces.
sonar.issue.ignore.multicriteria.c14.ruleKey=c:TabCharacter
sonar.issue.ignore.multicriteria.c14.resourceKey=**/*
# Complete the task associated to this TODO comment.
sonar.issue.ignore.multicriteria.c15.ruleKey=c:TodoTagPresence
sonar.issue.ignore.multicriteria.c15.resourceKey=**/*
# Split this 166 characters long line (which is greater than 160 authorized).
sonar.issue.ignore.multicriteria.c16.ruleKey=c:TooLongLine
sonar.issue.ignore.multicriteria.c16.resourceKey=**/*
# The number of code lines in this function is 212 which is greater than 200 authorized.
sonar.issue.ignore.multicriteria.c17.ruleKey=c:TooManyLinesOfCodeInFunction
sonar.issue.ignore.multicriteria.c17.resourceKey=**/*
# At most one statement is allowed per line, but 2 statements were found on this line.
sonar.issue.ignore.multicriteria.c18.ruleKey=c:TooManyStatementsPerLine
sonar.issue.ignore.multicriteria.c18.resourceKey=**/*
# parameter list has 9 parameters, which is greater than the 7 authorized.
sonar.issue.ignore.multicriteria.c19.ruleKey=c:TooManyParameters
sonar.issue.ignore.multicriteria.c19.resourceKey=**/*
# Undocumented API: hwloc_noos_component
sonar.issue.ignore.multicriteria.c20.ruleKey=c:UndocumentedApi
sonar.issue.ignore.multicriteria.c20.resourceKey=**/*
# Extra care should be taken to ensure that character arrays that are allocated on the stack are used safely. They are prime targets for buffer overflow attacks.
sonar.issue.ignore.multicriteria.r1.ruleKey=rats-c:fixed size global buffer
sonar.issue.ignore.multicriteria.r1.resourceKey=**/*
# A potential race condition vulnerability exists here. Normally a call to this function is vulnerable only when a match check precedes it. No check was detected, however one could still exist that could not be detected.
sonar.issue.ignore.multicriteria.r2.ruleKey=rats-c:fixed size local buffer
sonar.issue.ignore.multicriteria.r2.resourceKey=**/*
# Double check that your buffer is as big as you specify. When using functions that accept a number n of bytes to copy, such as strncpy, be aware that if the dest buffer size = n it may not NULL-terminate the string.sonar.issue.ignore.multicriteria.e8.ruleKey=rats:snprintf
sonar.issue.ignore.multicriteria.r3.ruleKey=rats-c:memcpy
sonar.issue.ignore.multicriteria.r3.resourceKey=**/*
# Don't use on memory intended to be secure, because the old structure will not be zeroed out
sonar.issue.ignore.multicriteria.r4.ruleKey=rats-c:realloc
sonar.issue.ignore.multicriteria.r4.resourceKey=**/*
# Double check that your buffer is as big as you specify. When using functions that accept a number n of bytes to copy, such as strncpy, be aware that if the dest buffer size = n it may not NULL-terminate the string.
sonar.issue.ignore.multicriteria.r5.ruleKey=rats-c:snprintf
sonar.issue.ignore.multicriteria.r5.resourceKey=**/*
# Check to be sure that the format string passed as argument 2 to this function call does not come from an untrusted source that could have added formatting characters that the code is not prepared to handle. Additionally, the format string could contain '%s' without precision that could result in a buffer overflow.
sonar.issue.ignore.multicriteria.r6.ruleKey=rats-c:sprintf
sonar.issue.ignore.multicriteria.r6.resourceKey=**/*
# This function does not properly handle non-NULL terminated strings. This does not result in exploitable code, but can lead to access violations.
sonar.issue.ignore.multicriteria.r7.ruleKey=rats-c:strlen
sonar.issue.ignore.multicriteria.r7.resourceKey=**/*
EOF

# Run the sonar-scanner analysis and submit to SonarQube server
sonar-scanner -X > sonar.log

exit 0
