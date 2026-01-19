# SPDX-License-Identifier: BSD-3-Clause
# Copyright © 2015-2026 Inria.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your tests files here.
cpuid_tests = \
	Intel-ArrowLake-CoreUltra5-225U.test \
	Intel-RaptorLake-Corei7-1370P.test \
	Intel-SapphireRapids-2xXeonMax9460.test \
	Intel-CPUID.1A-1p2co2t.test \
	Intel-CPUID.1F-Qemu-2p3d3c2t.test \
	Intel-CascadeLake-2xXeon6230.test \
	Intel-Skylake-2xXeon6140.test \
	Intel-Broadwell-2xXeon-E5-2650Lv4.test \
	Intel-Haswell-2xXeon-E5-2680v3.test \
	Intel-IvyBridge-12xXeon-E5-4620v2.test \
	Intel-SandyBridge-2xXeon-E5-2650.test \
	Intel-Westmere-2xXeon-X5650.test \
	Intel-Nehalem-2xXeon-X5550.test \
	Intel-Penryn-4xXeon-X7460.test \
	Intel-Core-2xXeon-E5345.test \
	Intel-KnightsLanding-XeonPhi-7210.test \
	Intel-KnightsCorner-XeonPhi-SE10P.test \
	AMD-19h-Zen4-2xEpyc-9654.test \
	AMD-19h-Zen3-2xEpyc-7763.test \
	AMD-17h-Zen-2xEpyc-7451.test \
	AMD-15h-Piledriver-4xOpteron-6348.test \
	AMD-15h-Bulldozer-4xOpteron-6272.test \
	AMD-K10-MagnyCours-2xOpteron-6164HE.test \
	AMD-K10-Istanbul-8xOpteron-8439SE.test \
	AMD-K8-SantaRosa-2xOpteron-2218.test \
	AMD-K8-SledgeHammer-2xOpteron-250.test \
	Hygon-Dhyana-32cores.test \
	Zhaoxin-CentaurHauls-ZXD-4600.test \
	Zhaoxin-2xCentaurHauls-KaiSheng-KH-40000.test \
	Zhaoxin-Shanghai-KaiSheng-ZXC+-FC1081.test

# Add your expected output file here.
# By default, it shows the output of `lstopo --of xml -'.
#
# XML outputs shouldn't have gp_index attributes to avoid spurious errors.
# Remove them with: sed -r -e 's/ gp_index=\"[0-9]+\"//' -i foo.outpu
cpuid_xml_outputs = \
	Intel-ArrowLake-CoreUltra5-225U.xml \
	Intel-RaptorLake-Corei7-1370P.xml \
	Intel-SapphireRapids-2xXeonMax9460.xml \
	Intel-CPUID.1A-1p2co2t.xml \
	Intel-CPUID.1F-Qemu-2p3d3c2t.xml \
	Intel-CascadeLake-2xXeon6230.xml \
	Intel-Skylake-2xXeon6140.xml \
	Intel-Broadwell-2xXeon-E5-2650Lv4.xml \
	Intel-Haswell-2xXeon-E5-2680v3.xml \
	Intel-IvyBridge-12xXeon-E5-4620v2.xml \
	Intel-SandyBridge-2xXeon-E5-2650.xml \
	Intel-Westmere-2xXeon-X5650.xml \
	Intel-Nehalem-2xXeon-X5550.xml \
	Intel-Penryn-4xXeon-X7460.xml \
	Intel-Core-2xXeon-E5345.xml \
	Intel-KnightsLanding-XeonPhi-7210.xml \
	Intel-KnightsCorner-XeonPhi-SE10P.xml \
	AMD-19h-Zen4-2xEpyc-9654.xml \
	AMD-19h-Zen3-2xEpyc-7763.xml \
	AMD-17h-Zen-2xEpyc-7451.xml \
	AMD-15h-Piledriver-4xOpteron-6348.xml \
	AMD-15h-Bulldozer-4xOpteron-6272.xml \
	AMD-K10-MagnyCours-2xOpteron-6164HE.xml \
	AMD-K10-Istanbul-8xOpteron-8439SE.xml \
	AMD-K8-SantaRosa-2xOpteron-2218.xml \
	AMD-K8-SledgeHammer-2xOpteron-250.xml \
	Hygon-Dhyana-32cores.xml \
	Zhaoxin-CentaurHauls-ZXD-4600.xml \
	Zhaoxin-2xCentaurHauls-KaiSheng-KH-40000.xml \
	Zhaoxin-Shanghai-KaiSheng-ZXC+-FC1081.xml

# FIXME add AMD-K10-Shanghai/Barcelona-[248]3xx

# Each output `xyz.output' must have a corresponding tarball `xyz.tar.bz2'
# specifying which tarball to use
cpuid_tarballs = \
	Intel-ArrowLake-CoreUltra5-225U.tar.bz2 \
	Intel-RaptorLake-Corei7-1370P.tar.bz2 \
	Intel-SapphireRapids-2xXeonMax9460.tar.bz2 \
	Intel-CPUID.1A-1p2co2t.tar.bz2 \
	Intel-CPUID.1F-Qemu-2p3d3c2t.tar.bz2 \
	Intel-CascadeLake-2xXeon6230.tar.bz2 \
	Intel-Skylake-2xXeon6140.tar.bz2 \
	Intel-Broadwell-2xXeon-E5-2650Lv4.tar.bz2 \
	Intel-Haswell-2xXeon-E5-2680v3.tar.bz2 \
	Intel-IvyBridge-12xXeon-E5-4620v2.tar.bz2 \
	Intel-SandyBridge-2xXeon-E5-2650.tar.bz2 \
	Intel-Westmere-2xXeon-X5650.tar.bz2 \
	Intel-Nehalem-2xXeon-X5550.tar.bz2 \
	Intel-Penryn-4xXeon-X7460.tar.bz2 \
	Intel-Core-2xXeon-E5345.tar.bz2 \
	Intel-KnightsLanding-XeonPhi-7210.tar.bz2 \
	Intel-KnightsCorner-XeonPhi-SE10P.tar.bz2 \
	AMD-19h-Zen4-2xEpyc-9654.tar.bz2 \
	AMD-19h-Zen3-2xEpyc-7763.tar.bz2 \
	AMD-17h-Zen-2xEpyc-7451.tar.bz2 \
	AMD-15h-Piledriver-4xOpteron-6348.tar.bz2 \
	AMD-15h-Bulldozer-4xOpteron-6272.tar.bz2 \
	AMD-K10-MagnyCours-2xOpteron-6164HE.tar.bz2 \
	AMD-K10-Istanbul-8xOpteron-8439SE.tar.bz2 \
	AMD-K8-SantaRosa-2xOpteron-2218.tar.bz2 \
	AMD-K8-SledgeHammer-2xOpteron-250.tar.bz2 \
	Hygon-Dhyana-32cores.tar.bz2 \
	Zhaoxin-CentaurHauls-ZXD-4600.tar.bz2 \
	Zhaoxin-2xCentaurHauls-KaiSheng-KH-40000.tar.bz2 \
	Zhaoxin-Shanghai-KaiSheng-ZXC+-FC1081.tar.bz2

if HWLOC_HAVE_BUNZIPP
TESTS = $(cpuid_tests)
endif HWLOC_HAVE_BUNZIPP

EXTRA_DIST = $(cpuid_tests) $(cpuid_xml_outputs) $(cpuid_tarballs)

# .test is in TEXT_EXTENSIONS by default, hence it uses its own test configuration
# variables prefixed with TEST_
TEST_LOG_COMPILER = $(abs_builddir)/test-topology.sh

check-local:
	if test -x extra/extra-tests.sh; then extra/extra-tests.sh "$(TEST_LOG_COMPILER)"; fi
