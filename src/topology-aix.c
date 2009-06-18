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

/* TODO: use SIGRECONFIG & dr_reconfig for state change */

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

#include <sys/rset.h>
#include <sys/processor.h>
#include <sys/thread.h>

static int
topo_aix_set_sth_cpubind(topo_topology_t topology, int what, int who, const topo_cpuset_t *topo_set, int strict)
{
  unsigned target;

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset)) {
    if (bindprocessor(what, who, PROCESSOR_CLASS_ANY))
      return -1;
    return 0;
  }

  /* TODO: use ra_attachrset instead to overcome this limitation, also provides
   * SHM memory binding and policy (P_FIRST_TOUCH / P_BALANCED)
   */
  if (topo_cpuset_weight(topo_set) != 1)
    return -1;

  target = topo_cpuset_first(topo_set);

  if (bindprocessor(what, who, target))
    return -1;

  return 0;
}

static int
topo_aix_set_thisproc_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_aix_set_sth_cpubind(topology, BINDPROCESS, getpid(), topo_set, strict);
}

static int
topo_aix_set_thisthread_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_aix_set_sth_cpubind(topology, BINDTHREAD, thread_self(), topo_set, strict);
}

static int
topo_aix_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *topo_set, int strict)
{
  return topo_aix_set_sth_cpubind(topology, BINDPROCESS, pid, topo_set, strict);
}

static int
topo_aix_set_thread_cpubind(topo_topology_t topology, topo_thread_t pthread, const topo_cpuset_t *topo_set, int strict)
{
  struct __pthrdsinfo info;
  int size;
  if (pthread_getthrds_np(&pthread, PTHRDSINFO_QUERY_TID, &info, sizeof(info), NULL, &size))
    return -1;
  return topo_aix_set_sth_cpubind(topology, BINDTHREAD, info.__pi_tid, topo_set, strict);
}

static int
topo_aix_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_aix_set_thisproc_cpubind(topology, topo_set, strict);
}

static void
look_rset(int sdl, enum topo_obj_type_e type, struct topo_topology *topology, int level)
{
  rsethandle_t rset, rad;
  int i,maxcpus,j;
  unsigned nbnodes;
  struct topo_obj *obj;

  if ((topology->flags & TOPO_FLAGS_WHOLE_SYSTEM))
    rset = rs_alloc(RS_ALL);
  else
    rset = rs_alloc(RS_PARTITION);
  rad = rs_alloc(RS_EMPTY);
  nbnodes = rs_numrads(rset, sdl, 0);
  if (nbnodes == -1) {
    perror("rs_numrads");
    return;
  }

  for (i = 0; i < nbnodes; i++) {
    if (rs_getrad(rset, rad, sdl, i, 0)) {
      fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
      continue;
    }
    if (!rs_getinfo(rad, R_NUMPROCS, 0))
      continue;

    /* It seems logical processors are numbered from 1 here, while the
     * bindprocessor functions numbers them from 0... */
    obj = topo_alloc_setup_object(type, i - (type == TOPO_OBJ_PROC));
    switch(type) {
      case TOPO_OBJ_NODE:
	obj->attr.node.memory_kB = 0; /* TODO */
	obj->attr.node.huge_page_free = 0;
	break;
      case TOPO_OBJ_CACHE:
	obj->attr.cache.memory_kB = 0; /* TODO */
	obj->attr.cache.depth = 2;
	break;
      case TOPO_OBJ_MISC:
	obj->attr.misc.depth = level;
      default:
	break;
    }
    maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
    for (j = 0; j < maxcpus; j++) {
      if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
	topo_cpuset_set(&obj->cpuset,j);
    }
    topo_debug("%s %d has cpuset %"TOPO_PRIxCPUSET"\n",
	       topo_obj_type_string(type),
	       i, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
    topo_add_object(topology, obj);
  }

  rs_free(rset);
  rs_free(rad);
}

void
topo_look_aix(struct topo_topology *topology)
{
  unsigned i;
  /* TODO: R_LGPGDEF/R_LGPGFREE for large pages */

  topology->set_cpubind = topo_aix_set_cpubind;
  topology->set_proc_cpubind = topo_aix_set_proc_cpubind;
  topology->set_thread_cpubind = topo_aix_set_thread_cpubind;
  topology->set_thisproc_cpubind = topo_aix_set_thisproc_cpubind;
  topology->set_thisthread_cpubind = topo_aix_set_thisthread_cpubind;

  for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++)
    {
      int known = 0;
#if 0
      if (i == rs_getinfo(NULL, R_SMPSDL, 0))
	/* Not enabled for now because I'm not sure what it corresponds to. On
	 * decrypthon it contains all the cpus. Is it a "machine" or a "system"
	 * level ?
	 */
	{
	  topo_debug("looking AIX \"SMP\" sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_MACHINE, topology, i);
	  known = 1;
	}
#endif
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  topo_debug("looking AIX node sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_NODE, topology, i);
	  known = 1;
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  topo_debug("looking AIX L2 sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_CACHE, topology, i);
	  known = 1;
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  topo_debug("looking AIX core sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_CORE, topology, i);
	  known = 1;
	}
#      endif
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	{
	  topo_debug("looking AIX max sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_PROC, topology, i);
	  known = 1;
	}

      /* Don't know how it should be rendered, make a misc object for it.  */
      if (!known)
	{
	  topo_debug("looking AIX unknown sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_MISC, topology, i);
	}
    }
}
