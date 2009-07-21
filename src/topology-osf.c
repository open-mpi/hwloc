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

#include <config.h>

#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <topology.h>
#include <topology/private.h>
#include <topology/debug.h>

#include <numa.h>
#include <radset.h>
#include <cpuset.h>

static int
prepare_radset(topo_topology_t topology, radset_t *radset, const topo_cpuset_t *topo_set)
{
  unsigned cpu;
  cpuset_t target_cpuset;
  cpuset_t cpuset, xor_cpuset;
  radid_t radid;
  int ret = 0;

  cpusetcreate(&target_cpuset);
  cpuemptyset(target_cpuset);
  topo_cpuset_foreach_begin(cpu, topo_set)
    cpuaddset(target_cpuset, cpu);
  topo_cpuset_foreach_end()

  cpusetcreate(&cpuset);
  cpusetcreate(&xor_cpuset);
  for (radid = 0; radid < topology->backend_params.osf.nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }
    cpuxorset(target_cpuset, cpuset, xor_cpuset);
    if (cpucountset(xor_cpuset) == 0) {
      /* Found it */
      radsetcreate(radset);
      rademptyset(*radset);
      radaddset(*radset, radid);
      ret = 1;
      goto out;
    }
  }
  /* radset containing exactly this set of CPUs not found */

out:
  cpusetdestroy(&target_cpuset);
  cpusetdestroy(&cpuset);
  cpusetdestroy(&xor_cpuset);
  return ret;
}

static int
topo_osf_set_thread_cpubind(topo_topology_t topology, topo_thread_t thread, const topo_cpuset_t *topo_set, int strict)
{
  radset_t radset;
  unsigned cpu;

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset)) {
    if (pthread_rad_detach(thread))
      return -1;
    return 0;
  }

  if (!prepare_radset(topology, &radset, topo_set))
    return -1;

  if (strict) {
    if (pthread_rad_bind(thread, radset, RAD_INSIST))
      return -1;
  } else {
    if (pthread_rad_attach(thread, radset, 0))
      return -1;
  }
  radsetdestroy(&radset);

  return 0;
}

static int
topo_osf_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *topo_set, int strict)
{
  radset_t radset;

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset)) {
    if (rad_detach_pid(pid))
      return -1;
    return 0;
  }

  if (!prepare_radset(topology, &radset, topo_set))
    return -1;

  if (strict) {
    if (rad_bind_pid(pid, radset, RAD_INSIST))
      return -1;
  } else {
    if (rad_attach_pid(pid, radset, 0))
      return -1;
  }
  radsetdestroy(&radset);

  return 0;
}

static int
topo_osf_set_thisthread_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_osf_set_thread_cpubind(topology, pthread_self(), topo_set, strict);
}

static int
topo_osf_set_thisproc_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_osf_set_proc_cpubind(topology, getpid(), topo_set, strict);
}

static int
topo_osf_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_osf_set_thisproc_cpubind(topology, topo_set, strict);
}

/* TODO: memory
 *
 * nloc to get distances
 *
 * nmadvise(addr,len), nmmap()
 * policies: DIRECTED, STRIPPED, first_touch(REPLICATED)
 *
 * nsg_init(), nsg_attach_pid(), RAD_MIGRATE/RAD_WAIT
 * assign_pid_to_pset()
 */

void
topo_look_osf(struct topo_topology *topology)
{
  cpu_cursor_t cursor;
  unsigned nbnodes;
  radid_t radid;
  cpuid_t cpuid;
  cpuset_t cpuset;
  struct topo_obj *obj;

  topology->set_cpubind = topo_osf_set_cpubind;
  topology->set_thread_cpubind = topo_osf_set_thread_cpubind;
  topology->set_thisthread_cpubind = topo_osf_set_thisthread_cpubind;
  topology->set_proc_cpubind = topo_osf_set_proc_cpubind;
  topology->set_thisproc_cpubind = topo_osf_set_thisproc_cpubind;

  topology->backend_params.osf.nbnodes = nbnodes = rad_get_num();

  cpusetcreate(&cpuset);
  for (radid = 0; radid < nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }

    obj = topo_alloc_setup_object(TOPO_OBJ_NODE, radid);
    obj->attr->node.memory_kB = rad_get_physmem(radid) / 1024;
    obj->attr->node.huge_page_free = 0;

    cursor = SET_CURSOR_INIT;
    while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
      topo_cpuset_set(&obj->cpuset,cpuid);

    topo_debug("node %d has cpuset %"TOPO_PRIxCPUSET"\n",
	       radid, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));

    topo_add_object(topology, obj);
  }

  /* add PROC objects */
  topo_setup_proc_level(topology, topo_fallback_nbprocessors(), NULL);
}
