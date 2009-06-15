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

#include <topology.h>
#include <topology/private.h>
#include <topology/debug.h>

#include <numa.h>
#include <radset.h>

static int
topo_osf_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  radset_t radset;
  unsigned cpu;

  radsetcreate(&radset);
  rademptyset(radset);
  topo_cpuset_foreach_begin(cpu, topo_set)
    radaddset(radset, cpu);
  topo_cpuset_foreach_end()
  if (pthread_rad_bind(pthread_self(), radset, RAD_INSIST))
    return -1;
  radsetdestroy(&radset);

  return 0;
}


/* TODO: process: bind_to_cpu(), bind_to_cpu_id(), rad_bind_pid(),
 * nsg_init(), nsg_attach_pid()
 * assign_pid_to_pset()
 * */

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

  nbnodes=rad_get_num();

  cpusetcreate(&cpuset);
  for (radid = 0; radid < nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }

    obj = topo_alloc_setup_object(TOPO_OBJ_NODE, radid);
    obj->attr.node.memory_kB = 0; /* TODO */
    obj->attr.node.huge_page_free = 0;

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
