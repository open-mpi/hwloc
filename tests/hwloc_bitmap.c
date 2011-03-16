/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2011 INRIA.  All rights reserved.
 * Copyright © 2009 Université Bordeaux 1
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <assert.h>

/* check misc bitmap stuff */

int main(void)
{
  hwloc_bitmap_t set;

  /* check an empty bitmap */
  set = hwloc_bitmap_alloc();
  assert(hwloc_bitmap_to_ulong(set) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == 0UL);
  /* check a non-empty bitmap */
  hwloc_bitmap_from_ith_ulong(set, 4, 0xff);
  assert(hwloc_bitmap_to_ith_ulong(set, 4) == 0xff);
  assert(hwloc_bitmap_to_ulong(set) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == 0UL);
  /* check a zeroed bitmap */
  hwloc_bitmap_zero(set);
  assert(hwloc_bitmap_to_ulong(set) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 4) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == 0UL);
  hwloc_bitmap_free(set);

  /* check a full bitmap */
  set = hwloc_bitmap_alloc_full();
  assert(hwloc_bitmap_to_ulong(set) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == ~0UL);
  /* check a almost full bitmap */
  hwloc_bitmap_set_ith_ulong(set, 4, 0xff);
  assert(hwloc_bitmap_to_ith_ulong(set, 4) == 0xff);
  assert(hwloc_bitmap_to_ulong(set) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == ~0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == ~0UL);
  /* check a almost empty bitmap */
  hwloc_bitmap_from_ith_ulong(set, 4, 0xff);
  assert(hwloc_bitmap_to_ith_ulong(set, 4) == 0xff);
  assert(hwloc_bitmap_to_ulong(set) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 0) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 1) == 0UL);
  assert(hwloc_bitmap_to_ith_ulong(set, 23) == 0UL);
  hwloc_bitmap_free(set);

  /* check ranges */
  set = hwloc_bitmap_alloc();
  assert(hwloc_bitmap_weight(set) == 0);
  /* 23-45 */
  hwloc_bitmap_set_range(set, 23, 45);
  assert(hwloc_bitmap_weight(set) == 23);
  /* 23-45,78- */
  hwloc_bitmap_set_range(set, 78, -1);
  assert(hwloc_bitmap_weight(set) == -1);
  /* 23- */
  hwloc_bitmap_set_range(set, 44, 79);
  assert(hwloc_bitmap_weight(set) == -1);
  assert(hwloc_bitmap_first(set) == 23);
  assert(!hwloc_bitmap_isfull(set));
  /* 0- */
  hwloc_bitmap_set_range(set, 0, 22);
  assert(hwloc_bitmap_weight(set) == -1);
  assert(hwloc_bitmap_isfull(set));
  /* 0-34,57- */
  hwloc_bitmap_clr_range(set, 35, 56);
  assert(hwloc_bitmap_weight(set) == -1);
  assert(!hwloc_bitmap_isfull(set));
  /* 0-34,57 */
  hwloc_bitmap_clr_range(set, 58, -1);
  assert(hwloc_bitmap_weight(set) == 36);
  assert(hwloc_bitmap_last(set) == 57);
  assert(hwloc_bitmap_next(set, 34) == 57);
  /* 0-34 */
  hwloc_bitmap_clr(set, 57);
  assert(hwloc_bitmap_weight(set) == 35);
  assert(hwloc_bitmap_last(set) == 34);
  /* empty */
  hwloc_bitmap_clr_range(set, 0, 34);
  assert(hwloc_bitmap_weight(set) == 0);
  assert(hwloc_bitmap_first(set) == -1);
  hwloc_bitmap_free(set);

  return 0;
}
