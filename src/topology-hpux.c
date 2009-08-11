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

/* TODO: memory:
 * mmap/shmget: +MAP/IPC_MEM_INTERLEAVED, MAP/IPC_MEM_LOCAL,
   MAP_IPC/MEM_FIRST_TOUCH */

/* TODO: psets?
 * since 11i 1.6:
   _SC_PSET_SUPPORT
   pset_create/destroy/assign/setattr
   pset_ctl/getattr
   pset_bind()
   pthread_pset_bind_np()
 */

#include <private/config.h>

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <topology.h>
#include <private/private.h>
#include <private/debug.h>

#include <sys/mpctl.h>
#include <pthread.h>

static ldom_t
topo_hpux_find_ldom(topo_topology_t topology, const topo_cpuset_t *topo_set)
{
  int has_numa = sysconf(_SC_CCNUMA_SUPPORT) == 1;
  int n;
  topo_obj_t objs[2];

  if (!has_numa)
    return -1;

  n = topo_get_cpuset_objs(topology, topo_set, objs, 2);
  if (n > 1 || objs[0]->type != TOPO_OBJ_NODE) {
    /* Does not correspond to exactly one node */
    return -1;
  }

  return objs[0]->os_index;
}

static spu_t
topo_hpux_find_spu(topo_topology_t topology, const topo_cpuset_t *topo_set)
{
  spu_t cpu;

  cpu = topo_cpuset_first(topo_set);
  if (cpu != -1 && topo_cpuset_weight(topo_set) == 1)
    return cpu;
  return -1;
}

static int
topo_hpux_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *topo_set, int strict)
{
  ldom_t ldom;
  spu_t cpu;

  /* Drop previous binding */
  mpctl(MPC_SETLDOM, MPC_LDOMFLOAT, pid);
  mpctl(MPC_SETPROCESS, MPC_SPUFLOAT, pid);

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset))
    return 0;

  ldom = topo_hpux_find_ldom(topology, topo_set);
  if (ldom != -1)
    return mpctl(MPC_SETLDOM, ldom, pid);

  cpu = topo_hpux_find_spu(topology, topo_set);
  if (cpu != -1)
    return mpctl(strict ? MPC_SETPROCESS_FORCE : MPC_SETPROCESS, cpu, pid);

  errno = EXDEV;
  return -1;
}

static int
topo_hpux_set_thisproc_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_hpux_set_proc_cpubind(topology, MPC_SELFPID, topo_set, strict);
}

static int
topo_hpux_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_hpux_set_thisproc_cpubind(topology, topo_set, strict);
}

static int
topo_hpux_set_thread_cpubind(topo_topology_t topology, topo_thread_t pthread, const topo_cpuset_t *topo_set, int strict)
{
  ldom_t ldom, ldom2;
  spu_t cpu, cpu2;

  /* Drop previous binding */
  pthread_ldom_bind_np(&ldom2, PTHREAD_LDOMFLOAT_NP, pthread);
  pthread_processor_bind_np(PTHREAD_BIND_ADVISORY_NP, &cpu2, PTHREAD_SPUFLOAT_NP, pthread);

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset))
    return 0;

  ldom = topo_hpux_find_ldom(topology, topo_set);
  if (ldom != -1)
    return pthread_ldom_bind_np(&ldom2, ldom, pthread);

  cpu = topo_hpux_find_spu(topology, topo_set);
  if (cpu != -1)
    return pthread_processor_bind_np(strict ? PTHREAD_BIND_FORCED_NP : PTHREAD_BIND_ADVISORY_NP, &cpu2, cpu, pthread);

  errno = EXDEV;
  return -1;
}

static int
topo_hpux_set_thisthread_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_hpux_set_thread_cpubind(topology, PTHREAD_SELFTID_NP, topo_set, strict);
}

void
topo_look_hpux(struct topo_topology *topology)
{
  int has_numa = sysconf(_SC_CCNUMA_SUPPORT) == 1;
  topo_obj_t *nodes = NULL, obj;
  spu_t currentcpu;
  ldom_t currentnode;
  int i, nbnodes;

  topology->set_cpubind = topo_hpux_set_cpubind;
  topology->set_proc_cpubind = topo_hpux_set_proc_cpubind;
  topology->set_thread_cpubind = topo_hpux_set_thread_cpubind;
  topology->set_thisproc_cpubind = topo_hpux_set_thisproc_cpubind;
  topology->set_thisthread_cpubind = topo_hpux_set_thisthread_cpubind;

  if (has_numa) {
    nbnodes = mpctl(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM ?
      MPC_GETNUMLDOMS_SYS : MPC_GETNUMLDOMS, 0, 0);

    topo_debug("%d nodes\n", nbnodes);

    nodes = malloc(nbnodes * sizeof(*nodes));

    i = 0;
    currentnode = mpctl(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM ?
      MPC_GETFIRSTLDOM_SYS : MPC_GETFIRSTLDOM, 0, 0);
    while (currentnode != -1 && i < nbnodes) {
      topo_debug("node %d is %d\n", i, currentnode);
      nodes[i] = obj = topo_alloc_setup_object(TOPO_OBJ_NODE, currentnode);
      /* TODO: obj->attr->node.memory_kB */
      /* TODO: obj->attr->node.huge_page_free */

      currentnode = mpctl(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM ?
        MPC_GETNEXTLDOM_SYS : MPC_GETNEXTLDOM, currentnode, 0);
      i++;
    }
  }

  i = 0;
  currentcpu = mpctl(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM ?
      MPC_GETFIRSTSPU_SYS : MPC_GETFIRSTSPU, 0,0);
  while (currentcpu != -1) {
    obj = topo_alloc_setup_object(TOPO_OBJ_PROC, currentcpu);
    topo_cpuset_set(&obj->cpuset, currentcpu);

    topo_debug("cpu %d\n", currentcpu);

    if (nodes) {
      /* Add this cpu to its node */
      currentnode = mpctl(MPC_SPUTOLDOM, currentcpu, 0);
      if (nodes[i]->os_index != currentnode)
        for (i = 0; i < nbnodes; i++)
          if (nodes[i]->os_index == currentnode)
            break;
      assert(i < nbnodes);
      topo_cpuset_set(&nodes[i]->cpuset, currentcpu);
      topo_debug("is in node %d\n", i);
    }

    /* Add cpu */
    topo_add_object(topology, obj);

    currentcpu = mpctl(topology->flags & TOPO_FLAGS_WHOLE_SYSTEM ?
      MPC_GETNEXTSPU_SYS : MPC_GETNEXTSPU, currentcpu, 0);
  }

  if (nodes) {
    /* Add nodes */
    for (i = 0 ; i < nbnodes ; i++)
      topo_add_object(topology, nodes[i]);
    free(nodes);
  }
}
