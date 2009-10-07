/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <assert.h>

/* check hwloc_cpuset_first(), _last() and _weight() */

int main()
{
  hwloc_cpuset_t set;

  /* empty set */
  set = hwloc_cpuset_alloc();
  assert(hwloc_cpuset_first(set) == -1);
  assert(hwloc_cpuset_last(set) == -1);
  assert(hwloc_cpuset_weight(set) == 0);

  /* full set */
  hwloc_cpuset_fill(set);
  assert(hwloc_cpuset_first(set) == 0);
  assert(hwloc_cpuset_last(set) == HWLOC_NBMAXCPUS-1);
  assert(hwloc_cpuset_weight(set) == HWLOC_NBMAXCPUS);

  /* custom sets */
  hwloc_cpuset_zero(set);
  hwloc_cpuset_set_range(set, 36, 59);
  assert(hwloc_cpuset_first(set) == 36);
  assert(hwloc_cpuset_last(set) == 59);
  assert(hwloc_cpuset_weight(set) == 24);
  hwloc_cpuset_set_range(set, 136, 259);
  assert(hwloc_cpuset_first(set) == 36);
  assert(hwloc_cpuset_last(set) == 259);
  assert(hwloc_cpuset_weight(set) == 148);
  hwloc_cpuset_clr(set, 199);
  assert(hwloc_cpuset_first(set) == 36);
  assert(hwloc_cpuset_last(set) == 259);
  assert(hwloc_cpuset_weight(set) == 147);

  hwloc_cpuset_free(set);

  return 0;
}
