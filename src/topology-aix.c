/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* TODO: use SIGRECONFIG & dr_reconfig for state change */

#include <private/config.h>

#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <sys/rset.h>
#include <sys/processor.h>
#include <sys/thread.h>

static int
hwloc_aix_set_sth_cpubind(hwloc_topology_t topology, rstype_t what, rsid_t who, hwloc_cpuset_t hwloc_set, int strict)
{
  rsethandle_t rset, rad;
  hwloc_obj_t objs[2];
  int n;
  int res = -1;

  if (hwloc_cpuset_isequal(hwloc_set, hwloc_get_system_obj(topology)->cpuset)) {
    if (ra_detachrset(what, who, 0))
      return -1;
    return 0;
  }

  n = hwloc_get_largest_objs_inside_cpuset(topology, hwloc_set, objs, 2);
  if (n > 1 || objs[0]->os_level == -1) {
    /* Does not correspond to exactly one radset, not possible */
    errno = EXDEV;
    return -1;
  }

  rset = rs_alloc(RS_PARTITION);
  rad = rs_alloc(RS_EMPTY);
  if (rs_getrad(rset, rad, objs[0]->os_level, objs[0]->os_index, 0)) {
    fprintf(stderr,"rs_getrad(%d,%d) failed: %s\n", objs[0]->os_level, objs[0]->os_index, strerror(errno));
    goto out;
  }

  /* TODO: ra_getrset to get binding information */
  /* TODO: memory binding and policy (P_DEFAULT / P_FIRST_TOUCH / P_BALANCED)
   * ra_mmap to allocation on an rset
   */

  if (ra_attachrset(what, who, rset, 0)) {
    res = -1;
    goto out;
  }

  res = 0;

out:
  rs_free(rset);
  rs_free(rad);
  return res;
}

static int
hwloc_aix_set_thisproc_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  rsid_t who = { .at_pid = getpid() };
  return hwloc_aix_set_sth_cpubind(topology, R_PROCESS, who, hwloc_set, strict);
}

static int
hwloc_aix_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  rsid_t who = { .at_tid = thread_self() };
  return hwloc_aix_set_sth_cpubind(topology, R_THREAD, who, hwloc_set, strict);
}

static int
hwloc_aix_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t hwloc_set, int strict)
{
  rsid_t who = { .at_pid = pid };
  return hwloc_aix_set_sth_cpubind(topology, R_PROCESS, who, hwloc_set, strict);
}

static int
hwloc_aix_set_thread_cpubind(hwloc_topology_t topology, hwloc_thread_t pthread, hwloc_cpuset_t hwloc_set, int strict)
{
  struct __pthrdsinfo info;
  int size;
  if (pthread_getthrds_np(&pthread, PTHRDSINFO_QUERY_TID, &info, sizeof(info), NULL, &size))
    return -1;
  rsid_t who = { .at_tid = info.__pi_tid };
  return hwloc_aix_set_sth_cpubind(topology, R_THREAD, who, hwloc_set, strict);
}

static int
hwloc_aix_set_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_aix_set_thisproc_cpubind(topology, hwloc_set, strict);
}

static void
look_rset(int sdl, hwloc_obj_type_t type, struct hwloc_topology *topology, int level)
{
  rsethandle_t rset, rad;
  int i,maxcpus,j;
  unsigned nbnodes;
  struct hwloc_obj *obj;

  if ((topology->flags & HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM))
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
    obj = hwloc_alloc_setup_object(type, i - (type == HWLOC_OBJ_PROC));
    obj->cpuset = hwloc_cpuset_alloc();
    obj->os_level = sdl;
    switch(type) {
      case HWLOC_OBJ_NODE:
	obj->attr->node.memory_kB = 0; /* TODO: odd, rs_getinfo(rad, R_MEMSIZE, 0) << 10 returns the total memory ... */
	obj->attr->node.huge_page_free = 0; /* TODO: rs_getinfo(rset, R_LGPGFREE, 0) / hugepagesize */
	break;
      case HWLOC_OBJ_CACHE:
	obj->attr->cache.memory_kB = 0; /* TODO: ? */
	obj->attr->cache.depth = 2;
	break;
      case HWLOC_OBJ_MISC:
	obj->attr->misc.depth = level;
      default:
	break;
    }
    maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
    for (j = 0; j < maxcpus; j++) {
      if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
	hwloc_cpuset_set(obj->cpuset, j);
    }
    hwloc_debug_2args_cpuset("%s %d has cpuset %s\n",
	       hwloc_obj_type_string(type),
	       i, obj->cpuset);
    hwloc_add_object(topology, obj);
  }

  rs_free(rset);
  rs_free(rad);
}

void
hwloc_look_aix(struct hwloc_topology *topology)
{
  unsigned i;
  /* TODO: R_LGPGDEF/R_LGPGFREE for large pages */

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
	  hwloc_debug("looking AIX \"SMP\" sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_MACHINE, topology, i);
	  known = 1;
	}
#endif
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  hwloc_debug("looking AIX node sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_NODE, topology, i);
	  known = 1;
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  hwloc_debug("looking AIX L2 sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_CACHE, topology, i);
	  known = 1;
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  hwloc_debug("looking AIX core sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_CORE, topology, i);
	  known = 1;
	}
#      endif
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	{
	  hwloc_debug("looking AIX max sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_PROC, topology, i);
	  known = 1;
	}

      /* Don't know how it should be rendered, make a misc object for it.  */
      if (!known)
	{
	  hwloc_debug("looking AIX unknown sdl %d\n", i);
	  look_rset(i, HWLOC_OBJ_MISC, topology, i);
	}
    }
}

void
hwloc_set_aix_hooks(struct hwloc_topology *topology)
{
  topology->set_cpubind = hwloc_aix_set_cpubind;
  topology->set_proc_cpubind = hwloc_aix_set_proc_cpubind;
  topology->set_thread_cpubind = hwloc_aix_set_thread_cpubind;
  topology->set_thisproc_cpubind = hwloc_aix_set_thisproc_cpubind;
  topology->set_thisthread_cpubind = hwloc_aix_set_thisthread_cpubind;
}
