# Copyright © 2009-2025 Inria.  All rights reserved.
# Copyright © 2009-2010 Université Bordeaux
# Copyright © 2009-2014 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your test files here.
xml_tests = \
        8intel64-4n2t-memattrs.test \
        16em64t-4s2c2t.test \
        16em64t-4s2c2t.console.test \
        16-2gr2gr2n2c+misc.test \
        16-2gr2gr2n2c+misc.console.test \
        16amd64-4distances.test \
        16amd64-4distances.console.test \
        16amd64-8n2c-cpusets.test \
        16em64t-4s2c2t-offlines.test \
        16intel64-manyVFs.test \
        16intel64-manyVFs.console.test \
        16intel64-manyVFs.nocollapse.console.test \
        24em64t-2n6c2t-pci.test \
        32em64t-2n8c2t-pci-noio.test \
        32em64t-2n8c2t-pci-normalio.test \
        32em64t-2n8c2t-pci-wholeio.test \
        irregulargroups-disallowed.test \
        irregulargroups-disallowed.console.test \
        64intel64-fakeKNL-SNC4-hybrid.test \
        96em64t-4n4d3ca2co-pci.test \
        192em64t-12gr2n8c2t.test \
        192em64t-24n8c2t.test \
        power8gpudistances.test \
        nvidiaDGX2.test \
        fakeheterodistances.test \
        fakecpukinds.test \
        memorysidecaches.test \
        cxlmem+dax.v2tov3.test \
        cxlmem+dax.v3tov2.test

# Add your XML files here.
xml_xml_outputs = \
        8intel64-4n2t-memattrs.xml \
        16em64t-4s2c2t.xml \
        16-2gr2gr2n2c+misc.xml \
        16amd64-4distances.xml \
        16amd64-8n2c-cpusets.xml \
        16em64t-4s2c2t-offlines.xml \
        16intel64-manyVFs.xml \
        24em64t-2n6c2t-pci.xml \
        32em64t-2n8c2t-pci-noio.xml \
        32em64t-2n8c2t-pci-normalio.xml \
        32em64t-2n8c2t-pci-wholeio.xml \
        irregulargroups-disallowed.xml \
        64intel64-fakeKNL-SNC4-hybrid.xml \
        96em64t-4n4d3ca2co-pci.xml \
        192em64t-12gr2n8c2t.xml \
        192em64t-24n8c2t.xml \
        power8gpudistances.xml \
        nvidiaDGX2.xml \
        fakeheterodistances.xml \
        fakecpukinds.xml \
        memorysidecaches.xml \
        cxlmem+dax.v2.xml \
        cxlmem+dax.v3.xml

# Add your console outputs here.
xml_console_outputs = \
        16em64t-4s2c2t.console \
        16-2gr2gr2n2c+misc.console \
        16amd64-4distances.console \
        16intel64-manyVFs.console \
        16intel64-manyVFs.nocollapse.console \
        irregulargroups-disallowed.console

# Only run the tests if we're building standalone, because the tests
# call hwloc executables.
TESTS = $(xml_tests)

EXTRA_DIST = $(xml_tests) $(xml_xml_outputs) $(xml_console_outputs)

# .test is in TEXT_EXTENSIONS by default, hence it uses its own test configuration
# variables prefixed with TEST_
TEST_LOG_COMPILER = $(abs_builddir)/test-topology.sh

check-local:
	if test -x extra/extra-tests.sh; then extra/extra-tests.sh "$(TEST_LOG_COMPILER)"; fi
