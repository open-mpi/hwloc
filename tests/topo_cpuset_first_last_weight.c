/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <topology.h>

#include <assert.h>

/* check topo_cpuset_first(), _last() and _weight() */

int main()
{
  topo_cpuset_t set;

  /* empty set */
  topo_cpuset_zero(&set);
  assert(topo_cpuset_first(&set) == -1);
  assert(topo_cpuset_last(&set) == -1);
  assert(topo_cpuset_weight(&set) == 0);

  /* full set */
  topo_cpuset_fill(&set);
  assert(topo_cpuset_first(&set) == 0);
  assert(topo_cpuset_last(&set) == TOPO_NBMAXCPUS-1);
  assert(topo_cpuset_weight(&set) == TOPO_NBMAXCPUS);

  /* custom sets */
  topo_cpuset_zero(&set);
  topo_cpuset_set_range(&set, 36, 59);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 59);
  assert(topo_cpuset_weight(&set) == 24);
  topo_cpuset_set_range(&set, 136, 259);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 259);
  assert(topo_cpuset_weight(&set) == 148);
  topo_cpuset_clr(&set, 199);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 259);
  assert(topo_cpuset_weight(&set) == 147);

  return 0;
}
