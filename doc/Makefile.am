# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am
include $(top_srcdir)/doxample.am

if DX_COND_man

dist_man3_MANS = \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_CPU.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_FULL.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_STRING_LENGTH.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_ZERO.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_CACHE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_CORE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_FAKE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_MACHINE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_NODE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_PROC.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_SOCKET.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_SYSTEM.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_TYPE_MAX.3 \
	@DX_DOCDIR@/man/man3/TOPO_PRIxCPUSET.3 \
	@DX_DOCDIR@/man/man3/TOPO_TYPE_DEPTH_MULTIPLE.3 \
	@DX_DOCDIR@/man/man3/TOPO_TYPE_DEPTH_UNKNOWN.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_foreach_begin.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_foreach_end.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_t.3 \
	@DX_DOCDIR@/man/man3/topo_find_closest_objs.3 \
	@DX_DOCDIR@/man/man3/topo_find_cpuset_covering_obj.3 \
	@DX_DOCDIR@/man/man3/topo_find_cpuset_objs.3 \
	@DX_DOCDIR@/man/man3/topo_get_depth_nbobjs.3 \
	@DX_DOCDIR@/man/man3/topo_get_depth_type.3 \
	@DX_DOCDIR@/man/man3/topo_get_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_type_depth.3 \
	@DX_DOCDIR@/man/man3/topo_obj.3 \
	@DX_DOCDIR@/man/man3/topo_obj_cpuset_snprintf.3 \
	@DX_DOCDIR@/man/man3/topo_obj_snprintf.3 \
	@DX_DOCDIR@/man/man3/topo_obj_t.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_e.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_string.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_t.3 \
	@DX_DOCDIR@/man/man3/topo_set_cpubind.3 \
	@DX_DOCDIR@/man/man3/topo_topology_destroy.3 \
	@DX_DOCDIR@/man/man3/topo_topology_get_info.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_all_keep_structure.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_type.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_type_keep_structure.3 \
	@DX_DOCDIR@/man/man3/topo_topology_info.3 \
	@DX_DOCDIR@/man/man3/topo_topology_init.3 \
	@DX_DOCDIR@/man/man3/topo_topology_load.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_flags.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_fsys_root.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_synthetic.3 \
	@DX_DOCDIR@/man/man3/topo_topology_t.3 \
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
	@DX_DOCDIR@/man/man3/arity.3 \
	@DX_DOCDIR@/man/man3/attr.3 \
	@DX_DOCDIR@/man/man3/children.3 \
	@DX_DOCDIR@/man/man3/cpuset.3 \
	@DX_DOCDIR@/man/man3/cpuset.h.3 \
	@DX_DOCDIR@/man/man3/depth.3 \
	@DX_DOCDIR@/man/man3/dmi_board_name.3 \
	@DX_DOCDIR@/man/man3/dmi_board_vendor.3 \
	@DX_DOCDIR@/man/man3/father.3 \
	@DX_DOCDIR@/man/man3/first_child.3 \
	@DX_DOCDIR@/man/man3/huge_page_size_kB.3 \
	@DX_DOCDIR@/man/man3/index.3 \
	@DX_DOCDIR@/man/man3/is_fake.3 \
	@DX_DOCDIR@/man/man3/last_child.3 \
	@DX_DOCDIR@/man/man3/level.3 \
	@DX_DOCDIR@/man/man3/next_cousin.3 \
	@DX_DOCDIR@/man/man3/next_sibling.3 \
	@DX_DOCDIR@/man/man3/number.3 \
	@DX_DOCDIR@/man/man3/physical_index.3 \
	@DX_DOCDIR@/man/man3/prev_cousin.3 \
	@DX_DOCDIR@/man/man3/prev_sibling.3 \
	@DX_DOCDIR@/man/man3/s.3 \
	@DX_DOCDIR@/man/man3/topology.h.3 \
	@DX_DOCDIR@/man/man3/type.3 \
	@DX_DOCDIR@/man/man3/userdata.3
endif

#info_TEXINFOS = libtopology.texi
