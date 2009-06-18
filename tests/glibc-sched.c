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

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <topology.h>
#include <topology/glibc-sched.h>

/* check the linux libnuma helpers */

int main(void)
{
  topo_topology_t topology;
  struct topo_topology_info topoinfo;
  topo_cpuset_t toposet;
  cpu_set_t schedset;
  topo_obj_t obj;
  int err;

  topo_topology_init(&topology);
  topo_topology_load(topology);
  topo_topology_get_info(topology, &topoinfo);

  toposet = topo_get_system_obj(topology)->cpuset;
  topo_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  topo_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  topo_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(topo_cpuset_isequal(&toposet, &topo_get_system_obj(topology)->cpuset));

  obj = topo_get_obj(topology, topoinfo.depth-1, topo_get_depth_nbobjs(topology, topoinfo.depth-1) - 1);
  assert(obj);
  assert(obj->type == TOPO_OBJ_PROC);

  toposet = obj->cpuset;
  topo_cpuset_to_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_setaffinity(0, sizeof(schedset));
#else
  err = sched_setaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);

  topo_cpuset_zero(&toposet);
#ifdef HAVE_OLD_SCHED_SETAFFINITY
  err = sched_getaffinity(0, sizeof(schedset));
#else
  err = sched_getaffinity(0, sizeof(schedset), &schedset);
#endif
  assert(!err);
  topo_cpuset_from_glibc_sched_affinity(topology, &toposet, &schedset, sizeof(schedset));
  assert(topo_cpuset_isequal(&toposet, &obj->cpuset));

  topo_topology_destroy(topology);
  return 0;
}
