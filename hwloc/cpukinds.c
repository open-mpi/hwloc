/*
 * Copyright © 2020 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "private/private.h"


/*****************
 * Basics
 */

void
hwloc_internal_cpukinds_init(struct hwloc_topology *topology)
{
  topology->cpukinds = NULL;
  topology->nr_cpukinds = 0;
  topology->nr_cpukinds_allocated = 0;
}

void
hwloc_internal_cpukinds_destroy(struct hwloc_topology *topology)
{
  unsigned i;
  for(i=0; i<topology->nr_cpukinds; i++) {
    struct hwloc_internal_cpukind_s *kind = &topology->cpukinds[i];
    hwloc_bitmap_free(kind->cpuset);
    hwloc__free_infos(kind->infos, kind->nr_infos);
  }
  free(topology->cpukinds);
  topology->cpukinds = NULL;
  topology->nr_cpukinds = 0;
}

int
hwloc_internal_cpukinds_dup(hwloc_topology_t new, hwloc_topology_t old)
{
  struct hwloc_tma *tma = new->tma;
  struct hwloc_internal_cpukind_s *kinds;
  unsigned i;

  kinds = hwloc_tma_malloc(tma, old->nr_cpukinds * sizeof(*kinds));
  if (!kinds)
    return -1;
  new->cpukinds = kinds;
  new->nr_cpukinds = old->nr_cpukinds;
  memcpy(kinds, old->cpukinds, old->nr_cpukinds * sizeof(*kinds));

  for(i=0;i<old->nr_cpukinds; i++) {
    kinds[i].cpuset = hwloc_bitmap_tma_dup(tma, old->cpukinds[i].cpuset);
    if (!kinds[i].cpuset) {
      new->nr_cpukinds = i;
      goto failed;
    }
    if (hwloc__tma_dup_infos(tma,
                             &kinds[i].infos, &kinds[i].nr_infos,
                             old->cpukinds[i].infos, old->cpukinds[i].nr_infos) < 0) {
      assert(!tma || !tma->dontfree); /* this tma cannot fail to allocate */
      hwloc_bitmap_free(kinds[i].cpuset);
      new->nr_cpukinds = i;
      goto failed;
    }
  }

  return 0;

 failed:
  hwloc_internal_cpukinds_destroy(new);
  return -1;
}

void
hwloc_internal_cpukinds_restrict(hwloc_topology_t topology)
{
  unsigned i;
  int removed = 0;
  for(i=0; i<topology->nr_cpukinds; i++) {
    struct hwloc_internal_cpukind_s *kind = &topology->cpukinds[i];
    hwloc_bitmap_and(kind->cpuset, kind->cpuset, hwloc_get_root_obj(topology)->cpuset);
    if (hwloc_bitmap_iszero(kind->cpuset)) {
      hwloc_bitmap_free(kind->cpuset);
      hwloc__free_infos(kind->infos, kind->nr_infos);
      memmove(kind, kind+1, (topology->nr_cpukinds - i - 1)*sizeof(*kind));
      i--;
      topology->nr_cpukinds--;
      removed = 1;
    }
  }
  if (removed)
    hwloc_internal_cpukinds_rank(topology);
}


/********************
 * Registering
 */

static __hwloc_inline void
hwloc__cpukind_add_infos(struct hwloc_internal_cpukind_s *kind,
                         const struct hwloc_info_s *infos, unsigned nr_infos)
{
  unsigned j;
  for(j=0; j<nr_infos; j++)
    hwloc__add_info(&kind->infos, &kind->nr_infos, infos[j].name, infos[j].value);
}

int
hwloc_internal_cpukinds_register(hwloc_topology_t topology, hwloc_cpuset_t cpuset,
                                 int forced_efficiency,
                                 const struct hwloc_info_s *infos, unsigned nr_infos,
                                 unsigned long flags)
{
  struct hwloc_internal_cpukind_s *kinds;
  unsigned i, max, bits, oldnr, newnr;

  if (hwloc_bitmap_iszero(cpuset)) {
    hwloc_bitmap_free(cpuset);
    errno = EINVAL;
    return -1;
  }

  if (flags & ~HWLOC_CPUKINDS_REGISTER_FLAG_OVERWRITE_FORCED_EFFICIENCY) {
    errno = EINVAL;
    return -1;
  }

  /* If we have N kinds currently, we may need 2N+1 kinds after inserting the new one:
   * - each existing kind may get split into which PUs are in the new kind and which aren't.
   * - some PUs might not have been in any kind yet.
   */
  max = 2 * topology->nr_cpukinds + 1;
  /* Allocate the power-of-two above 2N+1. */
  bits = hwloc_flsl(max-1) + 1;
  max = 1U<<bits;
  /* Allocate 8 minimum to avoid multiple reallocs */
  if (max < 8)
    max = 8;

  /* Create or enlarge the array of kinds if needed */
  kinds = topology->cpukinds;
  if (max > topology->nr_cpukinds_allocated) {
    kinds = realloc(kinds, max * sizeof(*kinds));
    if (!kinds) {
      hwloc_bitmap_free(cpuset);
      return -1;
    }
    memset(&kinds[topology->nr_cpukinds_allocated], 0, (max - topology->nr_cpukinds_allocated) * sizeof(*kinds));
    topology->nr_cpukinds_allocated = max;
    topology->cpukinds = kinds;
  }

  newnr = oldnr = topology->nr_cpukinds;
  for(i=0; i<oldnr; i++) {
    int res = hwloc_bitmap_compare_inclusion(cpuset, kinds[i].cpuset);
    if (res == HWLOC_BITMAP_INTERSECTS || res == HWLOC_BITMAP_INCLUDED) {
      /* new kind with intersection of cpusets and union of infos */
      kinds[newnr].cpuset = hwloc_bitmap_alloc();
      kinds[newnr].efficiency = HWLOC_CPUKIND_EFFICIENCY_UNKNOWN;
      kinds[newnr].forced_efficiency = forced_efficiency;
      hwloc_bitmap_and(kinds[newnr].cpuset, cpuset, kinds[i].cpuset);
      hwloc__cpukind_add_infos(&kinds[newnr], kinds[i].infos, kinds[i].nr_infos);
      hwloc__cpukind_add_infos(&kinds[newnr], infos, nr_infos);
      /* remove cpuset PUs from the existing kind that we just split */
      hwloc_bitmap_andnot(kinds[i].cpuset, kinds[i].cpuset, kinds[newnr].cpuset);
      /* clear cpuset PUs that were taken care of */
      hwloc_bitmap_andnot(cpuset, cpuset, kinds[newnr].cpuset);

      newnr++;

    } else if (res == HWLOC_BITMAP_CONTAINS
               || res == HWLOC_BITMAP_EQUAL) {
      /* append new info to existing smaller (or equal) kind */
      hwloc__cpukind_add_infos(&kinds[i], infos, nr_infos);
      if ((flags & HWLOC_CPUKINDS_REGISTER_FLAG_OVERWRITE_FORCED_EFFICIENCY)
          || kinds[i].forced_efficiency == HWLOC_CPUKIND_EFFICIENCY_UNKNOWN)
        kinds[i].forced_efficiency = forced_efficiency;
      /* clear cpuset PUs that were taken care of */
      hwloc_bitmap_andnot(cpuset, cpuset, kinds[i].cpuset);

    } else {
      assert(res == HWLOC_BITMAP_DIFFERENT);
      /* nothing to do */
    }

    /* don't compare with anything else if already empty */
    if (hwloc_bitmap_iszero(cpuset))
      break;
  }

  /* add a final kind with remaining PUs if any */
  if (!hwloc_bitmap_iszero(cpuset)) {
    kinds[newnr].cpuset = cpuset;
    kinds[newnr].efficiency = HWLOC_CPUKIND_EFFICIENCY_UNKNOWN;
    kinds[newnr].forced_efficiency = forced_efficiency;
    hwloc__cpukind_add_infos(&kinds[newnr], infos, nr_infos);
    newnr++;
  } else {
    hwloc_bitmap_free(cpuset);
  }

  topology->nr_cpukinds = newnr;
  return 0;
}


/*********************
 * Ranking
 */

int
hwloc_internal_cpukinds_rank(struct hwloc_topology *topology __hwloc_attribute_unused)
{
  /* TODO */
  return 0;
}


/*****************
 * Consulting
 */

int
hwloc_cpukinds_get_nr(hwloc_topology_t topology, unsigned long flags)
{
  if (flags) {
    errno = EINVAL;
    return -1;
  }

  return topology->nr_cpukinds;
}

int
hwloc_cpukinds_get_info(hwloc_topology_t topology,
                        unsigned id,
                        hwloc_bitmap_t cpuset,
                        int *efficiencyp,
                        unsigned *nr_infosp, struct hwloc_info_s **infosp,
                        unsigned long flags)
{
  struct hwloc_internal_cpukind_s *kind;

  if (flags) {
    errno = EINVAL;
    return -1;
  }

  if (id >= topology->nr_cpukinds) {
    errno = ENOENT;
    return -1;
  }

  kind = &topology->cpukinds[id];

  if (cpuset)
    hwloc_bitmap_copy(cpuset, kind->cpuset);

  if (efficiencyp)
    *efficiencyp = kind->efficiency;

  if (nr_infosp && infosp) {
    *nr_infosp = kind->nr_infos;
    *infosp = kind->infos;
  }
  return 0;
}

int
hwloc_cpukinds_get_by_cpuset(hwloc_topology_t topology,
                             hwloc_const_bitmap_t cpuset,
                             unsigned long flags)
{
  unsigned id;

  if (flags) {
    errno = EINVAL;
    return -1;
  }

  if (!cpuset || hwloc_bitmap_iszero(cpuset)) {
    errno = EINVAL;
    return -1;
  }

  for(id=0; id<topology->nr_cpukinds; id++) {
    struct hwloc_internal_cpukind_s *kind = &topology->cpukinds[id];
    int res = hwloc_bitmap_compare_inclusion(cpuset, kind->cpuset);
    if (res == HWLOC_BITMAP_EQUAL || res == HWLOC_BITMAP_INCLUDED) {
      return (int) id;
    } else if (res == HWLOC_BITMAP_INTERSECTS || res == HWLOC_BITMAP_CONTAINS) {
      errno = EXDEV;
      return -1;
    }
  }

  errno = ENOENT;
  return -1;
}
