# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am
include $(top_srcdir)/doxample.am

if DX_COND_man

dist_man3_MANS = \
	@DX_DOCDIR@/man/man3/topo_cpuset_t.3 \
	@DX_DOCDIR@/man/man3/topo_obj.3 \
	@DX_DOCDIR@/man/man3/topo_topology_info.3 \
	@DX_DOCDIR@/man/man3/topology_binding.3 \
	@DX_DOCDIR@/man/man3/topology_configuration.3 \
	@DX_DOCDIR@/man/man3/topology_conversion.3 \
	@DX_DOCDIR@/man/man3/topology_cpuset.3 \
	@DX_DOCDIR@/man/man3/topology_creation.3 \
	@DX_DOCDIR@/man/man3/topology_info.3 \
	@DX_DOCDIR@/man/man3/topology_information.3 \
	@DX_DOCDIR@/man/man3/topology_objects.3 \
	@DX_DOCDIR@/man/man3/topology_traversal.3

$(dist_man3_MANS): doxygen-doc

endif

MOSTLYCLEANFILES = $(DX_CLEANFILES)

EXTRA_DIST = $(DX_CONFIG)

if DX_COND_man
EXTRA_DIST += \
	@DX_DOCDIR@/man/man3/cpuset.h.3 \
	@DX_DOCDIR@/man/man3/topology.h.3
endif

#info_TEXINFOS = libtopology.texi
