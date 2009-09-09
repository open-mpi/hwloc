/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_cpuset_singlify() */

int main()
{
  topo_cpuset_t orig, expected;

  /* empty set gives empty set */
  topo_cpuset_zero(&orig);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  assert(!topo_cpuset_compar(&orig, &expected));

  /* full set gives first bit only */
  topo_cpuset_fill(&orig);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  topo_cpuset_set(&expected, 0);
  assert(!topo_cpuset_compar(&orig, &expected));

  /* actual non-trivial set */
  topo_cpuset_zero(&orig);
  topo_cpuset_set(&orig, 45);
  topo_cpuset_set(&orig, 46);
  topo_cpuset_set(&orig, 517);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  topo_cpuset_set(&expected, 45);
  assert(!topo_cpuset_compar(&orig, &expected));

  return 0;
}
