/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2010 INRIA
 * Copyright © 2009-2010 Université Bordeaux 1
 * Copyright © 2009-2010 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief The old deprecated Cpuset API.
 * This interface should not be used anymore, it will be dropped in a later release.
 *
 * hwloc/bitmap.h should be used instead. Most hwloc_cpuset_foo functions are
 * replaced with hwloc_bitmap_foo. The only exceptions are:
 * - hwloc_cpuset_from_string -> hwloc_bitmap_sscanf
 * - hwloc_cpuset_cpu -> hwloc_bitmap_only
 * - hwloc_cpuset_all_but_cpu -> hwloc_bitmap_allbut
 */

#ifndef HWLOC_CPUSET_H
#define HWLOC_CPUSET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hwloc/bitmap.h"

#define hwloc_cpuset_alloc hwloc_bitmap_alloc
#define hwloc_cpuset_free hwloc_bitmap_free
#define hwloc_cpuset_dup hwloc_bitmap_dup
#define hwloc_cpuset_copy hwloc_bitmap_copy
#define hwloc_cpuset_snprintf hwloc_bitmap_snprintf
#define hwloc_cpuset_asprintf hwloc_bitmap_asprintf
#define hwloc_cpuset_from_string hwloc_bitmap_sscanf
#define hwloc_cpuset_taskset_snprintf hwloc_bitmap_taskset_snprintf
#define hwloc_cpuset_taskset_asprintf hwloc_bitmap_taskset_asprintf
#define hwloc_cpuset_taskset_sscanf hwloc_bitmap_taskset_sscanf
#define hwloc_cpuset_zero hwloc_bitmap_zero
#define hwloc_cpuset_fill hwloc_bitmap_fill
#define hwloc_cpuset_from_ulong hwloc_bitmap_from_ulong
#define hwloc_cpuset_from_ith_ulong hwloc_bitmap_from_ith_ulong
#define hwloc_cpuset_to_ulong hwloc_bitmap_to_ulong
#define hwloc_cpuset_to_ith_ulong hwloc_bitmap_to_ith_ulong
#define hwloc_cpuset_cpu hwloc_bitmap_only
#define hwloc_cpuset_all_but_cpu hwloc_bitmap_allbut
#define hwloc_cpuset_set hwloc_bitmap_set
#define hwloc_cpuset_set_range hwloc_bitmap_set_range
#define hwloc_cpuset_set_ith_ulong hwloc_bitmap_set_ith_ulong
#define hwloc_cpuset_clr hwloc_bitmap_clr
#define hwloc_cpuset_clr_range hwloc_bitmap_clr_range
#define hwloc_cpuset_isset hwloc_bitmap_isset
#define hwloc_cpuset_iszero hwloc_bitmap_iszero
#define hwloc_cpuset_isfull hwloc_bitmap_isfull
#define hwloc_cpuset_isequal hwloc_bitmap_isequal
#define hwloc_cpuset_intersects hwloc_bitmap_intersects
#define hwloc_cpuset_isincluded hwloc_bitmap_isincluded
#define hwloc_cpuset_or hwloc_bitmap_or
#define hwloc_cpuset_and hwloc_bitmap_and
#define hwloc_cpuset_andnot hwloc_bitmap_andnot
#define hwloc_cpuset_xor hwloc_bitmap_xor
#define hwloc_cpuset_not hwloc_bitmap_not
#define hwloc_cpuset_first hwloc_bitmap_first
#define hwloc_cpuset_last hwloc_bitmap_last
#define hwloc_cpuset_next hwloc_bitmap_next
#define hwloc_cpuset_singlify hwloc_bitmap_singlify
#define hwloc_cpuset_compare_first hwloc_bitmap_compare_first
#define hwloc_cpuset_compare hwloc_bitmap_compare
#define hwloc_cpuset_weight hwloc_bitmap_weight
#define hwloc_cpuset_foreach_begin hwloc_bitmap_foreach_begin
#define hwloc_cpuset_foreach_end hwloc_bitmap_foreach_end

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HWLOC_CPUSET_H */
