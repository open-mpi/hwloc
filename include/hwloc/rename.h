/*
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_RENAME_H
#define HWLOC_RENAME_H

#include <hwloc/config.h>

/* Only enact these defines if we're actually renaming the symbols
   (i.e., avoid trying to have no-op defines if we're *not*
   renaming). */

#if HWLOC_SYM_TRANSFORM

/* Use a preprocessor two-step in order to get the prefixing right.
   Make 2 macros: HWLOC_NAME and HWLOC_NAME_CAPS for renaming
   things. */

#define HWLOC_MUNGE_NAME(a, b) HWLOC_MUNGE_NAME2(a, b)
#define HWLOC_MUNGE_NAME2(a, b) a ## b
#define HWLOC_NAME(name) HWLOC_MUNGE_NAME(HWLOC_SYM_PREFIX, hwloc_ ## name)
#define HWLOC_NAME_CAPS(name) HWLOC_MUNGE_NAME(HWLOC_SYM_PREFIX_CAPS, hwloc_ ## name)

/* Now define all the "real" names to be the prefixed names.  This
   allows us to use the real names throughout the code base (i.e.,
   "hwloc_<foo>"); the preprocessor will adjust to have the prefixed
   name under the covers. */

/* Names from hwloc.h */

#define hwloc_topology HWLOC_NAME(topology)
#define hwloc_topology_t HWLOC_NAME(topology_t)

#define HWLOC_OBJ_SYSTEM HWLOC_NAME_CAPS(OBJ_SYSTEM)
#define HWLOC_OBJ_MACHINE HWLOC_NAME_CAPS(OBJ_MACHINE)
#define HWLOC_OBJ_NODE HWLOC_NAME_CAPS(OBJ_NODE)
#define HWLOC_OBJ_SOCKET HWLOC_NAME_CAPS(OBJ_SOCKET)
#define HWLOC_OBJ_CACHE HWLOC_NAME_CAPS(OBJ_CACHE)
#define HWLOC_OBJ_CORE HWLOC_NAME_CAPS(OBJ_CORE)
#define HWLOC_OBJ_PROC HWLOC_NAME_CAPS(OBJ_PROC)
#define HWLOC_OBJ_MISC HWLOC_NAME_CAPS(OBJ_MISC)

#define hwloc_obj_type_t HWLOC_NAME(obj_type_t)

#define hwloc_compare_types HWLOC_NAME(compare_types)

#define HWLOC_TYPE_UNORDERED HWLOC_NAME_CAPS(TYPE_UNORDERED)

#define hwloc_obj HWLOC_NAME(obj)
#define hwloc_obj_t HWLOC_NAME(obj_t)

#define hwloc_obj_attr_u HWLOC_NAME(obj_attr_u)
#define hwloc_cache_attr_s HWLOC_NAME(cache_attr_s)
#define hwloc_memory_attr_s HWLOC_NAME(cache_memory_s)
#define hwloc_machine_attr_s HWLOC_NAME(cache_machine_s)
#define hwloc_misc_attr_s HWLOC_NAME(cache_misc_s)

#define hwloc_topology_init HWLOC_NAME(topology_init)
#define hwloc_topology_load HWLOC_NAME(topology_load)
#define hwloc_topology_destroy HWLOC_NAME(topology_destroy)
#define hwloc_topology_check HWLOC_NAME(topology_check)
#define hwloc_topology_ignore_type HWLOC_NAME(topology_ignore_type)
#define hwloc_topology_ignore_type_keep_structure HWLOC_NAME(topology_ignore_type_keep_structure)
#define hwloc_topology_ignore_all_keep_structure HWLOC_NAME(topology_ignore_all_keep_structure)

#define hwloc_topology_flags_e HWLOC_NAME(topology_flags_e)

#define HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM HWLOC_NAME_CAPS(TOPOLOGY_FLAG_WHOLE_SYSTEM)
#define HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM HWLOC_NAME_CAPS(TOPOLOGY_FLAG_IS_THISSYSTEM)

#define hwloc_topology_set_flags HWLOC_NAME(topology_set_flags)
#define hwloc_topology_set_fsroot HWLOC_NAME(topology_set_fsroot)
#define hwloc_topology_set_synthetic HWLOC_NAME(topology_set_synthetic)
#define hwloc_topology_set_xml HWLOC_NAME(topology_set_xml)

#define hwloc_topology_support_flags_e HWLOC_NAME(topology_support_flags_e)

#define HWLOC_SUPPORT_DISCOVERY HWLOC_NAME_CAPS(SUPPORT_DISCOVERY)
#define HWLOC_SUPPORT_SET_PROC_CPUBIND HWLOC_NAME_CAPS(SUPPORT_SET_PROC_CPUBIND)
#define HWLOC_SUPPORT_SET_THREAD_CPUBIND HWLOC_NAME_CAPS(SUPPORT_SET_THREAD_CPUBIND)
#define HWLOC_SUPPORT_GET_PROC_CPUBIND HWLOC_NAME_CAPS(SUPPORT_GET_PROC_CPUBIND)
#define HWLOC_SUPPORT_GET_THREAD_CPUBIND HWLOC_NAME_CAPS(SUPPORT_GET_THREAD_CPUBIND)

#define hwloc_topology_get_support HWLOC_NAME(topology_get_support)
#define hwloc_topology_export_xml HWLOC_NAME(topology_export_xml)
#define hwloc_topology_get_depth HWLOC_NAME(topology_get_depth)
#define hwloc_topology_get_type_depth HWLOC_NAME(topology_get_type_depth)

#define HWLOC_TYPE_DEPTH_UNKNOWN HWLOC_NAME_CAPS(TYPE_DEPTH_UNKNOWN)
#define HWLOC_TYPE_DEPTH_MULTIPLE HWLOC_NAME_CAPS(TYPE_DEPTH_MULTIPLE)

#define hwloc_get_depth_type HWLOC_NAME(get_type_depth)
#define hwloc_get_nobjs_by_depth HWLOC_NAME(get_nobjs_by_depth)
#define hwloc_get_nobjs_by_type HWLOC_NAME(get_nobjs_by_type)

#define hwloc_topology_is_thissystem HWLOC_NAME(topology_is_thissystem)
#define hwloc_topology_get_complete_cpuset HWLOC_NAME(topology_get_complete_cpuset)
#define hwloc_topology_get_topology_cpuset HWLOC_NAME(topology_get_topology_cpuset)
#define hwloc_topology_get_online_cpuset HWLOC_NAME(topology_get_online_cpuset)

#define hwloc_topology_get_allowed_cpuset HWLOC_NAME(topology_get_allowed_cpuset)
#define hwloc_get_obj_by_depth HWLOC_NAME(get_obj_by_depth )
#define hwloc_get_obj_by_type HWLOC_NAME(get_obj_by_type )

#define hwloc_obj_type_string HWLOC_NAME(obj_type_string )
#define hwloc_obj_type_of_string HWLOC_NAME(obj_type_of_string )
#define hwloc_obj_snprintf HWLOC_NAME(obj_snprintf)
#define hwloc_obj_cpuset_snprintf HWLOC_NAME(obj_cpuset_snprintf)

#define HWLOC_CPUBIND_PROCESS HWLOC_NAME_CAPS(CPUBIND_PROCESS)
#define HWLOC_CPUBIND_THREAD HWLOC_NAME_CAPS(CPUBIND_THREAD)
#define HWLOC_CPUBIND_STRICT HWLOC_NAME_CAPS(CPUBIND_STRICT)

#define hwloc_cpubind_policy_t HWLOC_NAME(hwloc_cpubind_policy_t)

#define hwloc_set_cpubind HWLOC_NAME(set_cpubind)
#define hwloc_get_cpubind HWLOC_NAME(get_cpubind)
#define hwloc_set_proc_cpubind HWLOC_NAME(set_proc_cpubind)
#define hwloc_get_proc_cpubind HWLOC_NAME(get_proc_cpubind)
#define hwloc_set_thread_cpubind HWLOC_NAME(set_thread_cpubind)
#define hwloc_get_thread_cpubind HWLOC_NAME(get_thread_cpubind)

/* hwloc/cpuset.h */

#define hwloc_cpuset HWLOC_NAME(cpuset)
#define hwloc_cpuset_s HWLOC_NAME(cpuset_s)
#define hwloc_cpuset_t HWLOC_NAME(cpuset_t)
#define hwloc_const_cpuset_t HWLOC_NAME(const_cpuset_t)

#define hwloc_cpuset_alloc HWLOC_NAME(cpuset_alloc)
#define hwloc_cpuset_free HWLOC_NAME(cpuset_free)
#define hwloc_cpuset_dup HWLOC_NAME(cpuset_dup)
#define hwloc_cpuset_copy HWLOC_NAME(cpuset_copy)
#define hwloc_cpuset_snprintf HWLOC_NAME(cpuset_snprintf)
#define hwloc_cpuset_asprintf HWLOC_NAME(cpuset_asprintf)
#define hwloc_cpuset_from_string HWLOC_NAME(cpuset_from_string)
#define hwloc_cpuset_zero HWLOC_NAME(cpuset_zero)
#define hwloc_cpuset_fill HWLOC_NAME(cpuset_fill)
#define hwloc_cpuset_from_ulong HWLOC_NAME(cpuset_from_ulong)

#define hwloc_cpuset_from_ith_ulong HWLOC_NAME(cpuset_from_ith_ulong)
#define hwloc_cpuset_to_ulong HWLOC_NAME(cpuset_to_ulong)
#define hwloc_cpuset_to_ith_ulong HWLOC_NAME(cpuset_to_ith_ulong)
#define hwloc_cpuset_cpu HWLOC_NAME(cpuset_cpu)
#define hwloc_cpuset_all_but_cpu HWLOC_NAME(cpuset_all_but_cpu)
#define hwloc_cpuset_set HWLOC_NAME(cpuset_set)
#define hwloc_cpuset_set_range HWLOC_NAME(cpuset_set_range)
#define hwloc_cpuset_clr HWLOC_NAME(cpuset_clr)
#define hwloc_cpuset_clr_range HWLOC_NAME(cpuset_clr_range)
#define hwloc_cpuset_isset HWLOC_NAME(cpuset_isset)
#define hwloc_cpuset_iszero HWLOC_NAME(cpuset_iszero)
#define hwloc_cpuset_isfull HWLOC_NAME(cpuset_isfull)
#define hwloc_cpuset_isequal HWLOC_NAME(cpuset_isequal)
#define hwloc_cpuset_intersects HWLOC_NAME(cpuset_intersects)
#define hwloc_cpuset_isincluded HWLOC_NAME(cpuset_isincluded)
#define hwloc_cpuset_orset HWLOC_NAME(cpuset_orset)
#define hwloc_cpuset_andset HWLOC_NAME(cpuset_andset)
#define hwloc_cpuset_clearset HWLOC_NAME(cpuset_clearset)
#define hwloc_cpuset_xorset HWLOC_NAME(cpuset_xorset)
#define hwloc_cpuset_notset HWLOC_NAME(cpuset_notset)
#define hwloc_cpuset_first HWLOC_NAME(cpuset_first)
#define hwloc_cpuset_last HWLOC_NAME(cpuset_last)
#define hwloc_cpuset_singlify HWLOC_NAME(cpuset_singlify)
#define hwloc_cpuset_compare_first HWLOC_NAME(cpuset_compare_first)
#define hwloc_cpuset_compare HWLOC_NAME(cpuset_compare)
#define hwloc_cpuset_weight HWLOC_NAME(cpuset_weight)

/* hwloc/helper.h */

#define hwloc_get_type_or_below_depth HWLOC_NAME(get_type_or_below_depth)
#define hwloc_get_type_or_above_depth HWLOC_NAME(get_type_or_above_depth)
#define hwloc_get_root_obj HWLOC_NAME(get_root_obj)
#define hwloc_get_ancestor_obj_by_depth HWLOC_NAME(get_ancestor_obj_by_depth)
#define hwloc_get_ancestor_obj_by_type HWLOC_NAME(get_ancestor_obj_by_type)
#define hwloc_get_next_obj_by_depth HWLOC_NAME(get_next_obj_by_depth)
#define hwloc_get_next_obj_by_type HWLOC_NAME(get_next_obj_by_type)
#define hwloc_get_proc_obj_by_os_index HWLOC_NAME(get_proc_obj_by_os_index)
#define hwloc_get_next_child HWLOC_NAME(get_next_child)
#define hwloc_get_common_ancestor_obj HWLOC_NAME(get_common_ancestor_obj)
#define hwloc_obj_is_in_subtree HWLOC_NAME(obj_is_in_subtree)

/* glibc-sched.h */

#define hwloc_cpuset_to_glibc_sched_affinity HWLOC_NAME(cpuset_to_glibc_sched_affinity)
#define hwloc_cpuset_from_glibc_sched_affinity HWLOC_NAME(cpuset_from_glibc_sched_affinity)

/* linux-libnuma.h */

#define hwloc_cpuset_to_linux_libnuma_ulongs HWLOC_NAME(cpuset_to_linux_libnuma_ulongs)
#define hwloc_cpuset_from_linux_libnuma_ulongs HWLOC_NAME(cpuset_from_linux_libnuma_ulongs)
#define hwloc_cpuset_to_linux_libnuma_bitmask HWLOC_NAME(cpuset_to_linux_libnuma_bitmask)
#define hwloc_cpuset_from_linux_libnuma_bitmask HWLOC_NAME(cpuset_from_linux_libnuma_bitmask)
#define hwloc_cpuset_to_linux_libnuma_nodemask HWLOC_NAME(cpuset_to_linux_libnuma_nodemask)
#define hwloc_cpuset_from_linux_libnuma_nodemask HWLOC_NAME(cpuset_from_linux_libnuma_nodemask)

/* linux.h */

#define hwloc_linux_parse_cpumap_file HWLOC_NAME(linux_parse_cpumap_file)
#define hwloc_linux_set_tid_cpubind HWLOC_NAME(linux_set_tid_cpubind)
#define hwloc_linux_get_tid_cpubind HWLOC_NAME(linux_get_tid_cpubind)

/* openfabrics-verbs.h */

#define hwloc_ibv_get_device_cpuset HWLOC_NAME(ibv_get_device_cpuset)

#endif

#endif /* HWLOC_RENAME_H */
