/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

/** \file
 * \brief Macros to help interaction between libtopolopy and glibc scheduling routines.
 *
 * Applications that use both libtopology and glibc scheduling routines such as
 * sched_getaffinity may want to include this file so as to ease conversion
 * between their respective types.
 */

#ifndef TOPOLOGY_GLIBC_SCHED_H
#define TOPOLOGY_GLIBC_SCHED_H

#include <topology.h>
#include <topology/helper.h>

#if !defined _GNU_SOURCE || !defined _SCHED_H
#error sched.h must be included with _GNU_SOURCE defined
#endif

#ifdef CPU_SET


/** \defgroup topology_glibc_sched Helpers for manipulating glibc sched affinity
 * @{
 */


/** \brief Convert libtopology CPU set \p toposet into glibc sched affinity CPU set \p schedset
 *
 * This function may be used before calling sched_setaffinity or any other function
 * that takes a cpu_set_t as input parameter.
 *
 * \p schedsetsize should be sizeof(cpu_set_t) unless \p schedset was dynamically allocated with CPU_ALLOC
 */
static __inline__ void
topo_cpuset_to_glibc_sched_affinity(topo_topology_t topology, const topo_cpuset_t *toposet,
				    cpu_set_t *schedset, size_t schedsetsize)
{
#ifdef CPU_ZERO_S
  unsigned cpu;
  CPU_ZERO_S(schedsetsize, schedset);
  topo_cpuset_foreach_begin(cpu, toposet)
    CPU_SET_S(cpu, schedsetsize, schedset);
  topo_cpuset_foreach_end();
#else /* !CPU_ZERO_S */
  unsigned cpu;
  CPU_ZERO(schedset);
  assert(schedsetsize == sizeof(cpu_set_t));
  topo_cpuset_foreach_begin(cpu, toposet)
    CPU_SET(cpu, schedset);
  topo_cpuset_foreach_end();
#endif /* !CPU_ZERO_S */
}

/** \brief Convert libtopology CPU set \p toposet into glibc sched affinity CPU set \p schedset
 *
 * This function may be used before calling sched_setaffinity  or any other function
 * that takes a cpu_set_t  as input parameter.
 *
 * \p schedsetsize should be sizeof(cpu_set_t) unless \p schedset was dynamically allocated with CPU_ALLOC
 */
static __inline__ void
topo_cpuset_from_glibc_sched_affinity(topo_topology_t topology, topo_cpuset_t *toposet,
				      const cpu_set_t *schedset, size_t schedsetsize)
{
#ifdef CPU_ZERO_S
  int cpu, count;
  topo_cpuset_zero(toposet);
  count = CPU_COUNT_S(schedsetsize, schedset);
  cpu = 0;
  while (count) {
    if (CPU_ISSET_S(cpu, schedsetsize, schedset)) {
      topo_cpuset_set(toposet, cpu);
      count--;
    }
    cpu++;
    if (cpu > TOPO_NBMAXCPUS)
      break;
  }
#else /* !CPU_ZERO_S */
  /* sched.h does not support dynamic cpu_set_t (introduced in glibc 2.7),
   * assume we have a very old interface without CPU_COUNT (added in 2.6)
   */
  int cpu;
  assert(schedsetsize == sizeof(cpu_set_t));
  for(cpu=0; cpu<CPU_SETSIZE && cpu<TOPO_NBMAXCPUS; cpu++)
    if (CPU_ISSET(cpu, schedset))
      topo_cpuset_set(toposet, cpu);
#endif /* !CPU_ZERO_S */
}

/** @} */


#endif /* CPU_SET */

#endif /* TOPOLOGY_GLIBC_SCHED_H */
