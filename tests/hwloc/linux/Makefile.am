# Copyright © 2009-2023 Inria.  All rights reserved.
# Copyright © 2009-2011 Université Bordeaux
# Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
# See COPYING in top-level directory.

AM_CFLAGS = $(HWLOC_CFLAGS)
AM_CPPFLAGS = $(HWLOC_CPPFLAGS)
AM_LDFLAGS = $(HWLOC_LDFLAGS)

# Add your tests here
sysfs_tests = \
	2arm-2c.test \
	2i386-2c-nohugepage.test \
	2i386-2t-hugepagesizecount.test \
	2pa-pcidomain32bits.test \
	2ps3-2t.test \
	4fake-4gr1nu1pu.test \
	8em64t-2s2ca2c-buggynuma.test \
	8em64t-2s2ca2c.test \
	8em64t-2s4c-asymcaches.test \
	8ia64-2n2s2c.test \
	8ia64-2s2c2t.test \
	16amd64-4n4c-cgroup-distance-merge.test \
	16amd64-8n2c-cpusets-disallowed.test \
	16amd64-8n2c-cpusets-xml.test \
	16amd64-8n2c-cpusets.test \
	16amd64-8n2c.test \
	16em64t-4s2c2t-merge.test \
	16em64t-4s2c2t-nocaches.test \
	16em64t-4s2c2t-offlines-xml.test \
	16em64t-4s2c2t-offlines.test \
	16em64t-4s2c2t-xml.test \
	16em64t-4s2c2t.test \
	16em64t-4s2ca2c-cpusetreorder-merge.test \
	16em64t-4s2ca2c-cpusetreorder.test \
	16ia64-8n2s.test \
	20s390-2g6s4c.test \
	32amd64-4s2n4c-cgroup2.test \
	32em64t-2n8c+dax+nvme+mic+dimms.test \
	32intel64-2p8co2t+8ve.test \
	40intel64-2g2n4c+pcilocality.test \
	40intel64-4n10c+pci-conflicts.test \
	48amd64-4pa2n6c-sparse.test \
	64amd64-4s2n4ca2co.test \
	64fake-4n2s2ca2c2t.test \
	64intel64-fakeKNL-A2A-cache.test \
	64intel64-fakeKNL-SNC4-hybrid-msc.test \
	64intel64-fakeKNL-SNC4-hybrid.test \
	96em64t-4no4pa3ca2co.test \
	128arm-2pa2n8cluster4co.test \
	128ia64-17n4s2c.test \
	256ia64-64n2s2c.test \
	256ppc-8n8s4t.test \
	fakecpuid1f-64intel64-2p4d2n2c2t.test \
	fakeheterocpunuma.test \
	fakeheteromemtiers.test \
	fakememinitiators-1np2c+1npp+gi.test \
	memorysidecaches.test \
	nvidiagpunumanodes.test \
	nvidiagpunumanodes-kept.test \
	offline-cpu0-node0.test

# Add your tests' input tarballs here
sysfs_tarballs = \
	2arm-2c.tar.bz2 \
	2i386-2c-nohugepage.tar.bz2 \
	2i386-2t-hugepagesizecount.tar.bz2 \
	2pa-pcidomain32bits.tar.bz2 \
	2ps3-2t.tar.bz2 \
	4fake-4gr1nu1pu.tar.bz2 \
	8em64t-2s2ca2c-buggynuma.tar.bz2 \
	8em64t-2s2ca2c.tar.bz2 \
	8em64t-2s4c-asymcaches.tar.bz2 \
	8ia64-2n2s2c.tar.bz2 \
	8ia64-2s2c2t.tar.bz2 \
	16amd64-4n4c-cgroup-distance-merge.tar.bz2 \
	16amd64-8n2c-cpusets.tar.bz2 \
	16amd64-8n2c.tar.bz2 \
	16em64t-4s2c2t.tar.bz2 \
	16em64t-4s2c2t-offlines.tar.bz2 \
	16em64t-4s2c2t-offlines.tar.bz2 \
	16em64t-4s2c2t.tar.bz2 \
	16em64t-4s2ca2c-cpusetreorder.tar.bz2 \
	16ia64-8n2s.tar.bz2 \
	20s390-2g6s4c.tar.bz2 \
	32amd64-4s2n4c-cgroup2.tar.bz2 \
	32em64t-2n8c+dax+nvme+mic+dimms.tar.bz2 \
	32intel64-2p8co2t+8ve.tar.bz2 \
	40intel64-2g2n4c+pcilocality.tar.bz2 \
	40intel64-4n10c+pci-conflicts.tar.bz2 \
	48amd64-4pa2n6c-sparse.tar.bz2 \
	64amd64-4s2n4ca2co.tar.bz2 \
	64fake-4n2s2ca2c2t.tar.bz2 \
	64intel64-fakeKNL-A2A-cache.tar.bz2 \
	64intel64-fakeKNL-SNC4-hybrid.tar.bz2 \
	96em64t-4no4pa3ca2co.tar.bz2 \
	128arm-2pa2n8cluster4co.tar.bz2 \
	128ia64-17n4s2c.tar.bz2 \
	256ia64-64n2s2c.tar.bz2 \
	256ppc-8n8s4t.tar.bz2 \
	fakecpuid1f-64intel64-2p4d2n2c2t.tar.bz2 \
	fakeheterocpunuma.tar.bz2 \
	fakeheteromemtiers.tar.bz2 \
	fakememinitiators-1np2c+1npp+gi.tar.bz2 \
	memorysidecaches.tar.bz2 \
	nvidiagpunumanodes.tar.bz2 \
	offline-cpu0-node0.tar.bz2

# Add your expected console outputs here.
sysfs_console_outputs = \
	2arm-2c.console \
	2pa-pcidomain32bits.console \
	2ps3-2t.console \
	4fake-4gr1nu1pu.console \
	8em64t-2s2ca2c-buggynuma.console \
	8em64t-2s2ca2c.console \
	8em64t-2s4c-asymcaches.console \
	8ia64-2n2s2c.console \
	8ia64-2s2c2t.console \
	16amd64-4n4c-cgroup-distance-merge.console \
	16amd64-8n2c-cpusets-disallowed.console \
	16amd64-8n2c-cpusets.console \
	16amd64-8n2c.console \
	16em64t-4s2c2t-merge.console \
	16em64t-4s2c2t-nocaches.console \
	16em64t-4s2c2t-offlines.console \
	16em64t-4s2c2t.console \
	16em64t-4s2ca2c-cpusetreorder.console \
	16em64t-4s2ca2c-cpusetreorder-merge.console \
	16ia64-8n2s.console \
	20s390-2g6s4c.console \
	32intel64-2p8co2t+8ve.console \
	40intel64-4n10c+pci-conflicts.console \
	48amd64-4pa2n6c-sparse.console \
	64amd64-4s2n4ca2co.console \
	64fake-4n2s2ca2c2t.console \
	64intel64-fakeKNL-A2A-cache.console \
	64intel64-fakeKNL-SNC4-hybrid-msc.console \
	64intel64-fakeKNL-SNC4-hybrid.console \
	96em64t-4no4pa3ca2co.console \
	128arm-2pa2n8cluster4co.console \
	128ia64-17n4s2c.console \
	256ia64-64n2s2c.console \
	256ppc-8n8s4t.console \
	fakecpuid1f-64intel64-2p4d2n2c2t.console \
	fakeheteromemtiers.console \
	fakememinitiators-1np2c+1npp+gi.console \
	memorysidecaches.console \
	nvidiagpunumanodes.console \
	nvidiagpunumanodes-kept.console \
	offline-cpu0-node0.console

# Add your expected console outputs here.
sysfs_xml_outputs = \
	2i386-2c-nohugepage.xml \
	2i386-2t-hugepagesizecount.xml \
	16amd64-8n2c-cpusets-xml.xml \
	16em64t-4s2c2t-offlines-xml.xml \
	16em64t-4s2c2t-xml.xml \
	32amd64-4s2n4c-cgroup2.xml \
	32em64t-2n8c+dax+nvme+mic+dimms.xml \
	40intel64-2g2n4c+pcilocality.xml \
	fakeheterocpunuma.xml

if HWLOC_HAVE_OPENAT
if HWLOC_HAVE_BUNZIPP
TESTS = $(sysfs_tests)
endif HWLOC_HAVE_BUNZIPP
endif HWLOC_HAVE_OPENAT

EXTRA_DIST = $(sysfs_tests) $(sysfs_tarballs) $(sysfs_console_outputs) $(sysfs_xml_outputs)

# .test is in TEXT_EXTENSIONS by default, hence it uses its own test configuration
# variables prefixed with TEST_
TEST_LOG_COMPILER = $(abs_builddir)/test-topology.sh

check-local:
	if test -x extra/extra-tests.sh; then extra/extra-tests.sh "$(TEST_LOG_COMPILER)"; fi

SUBDIRS = . allowed gather
