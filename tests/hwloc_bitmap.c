/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2010 INRIA
 * Copyright © 2009 Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
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

  return 0;
}
