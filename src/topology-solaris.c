/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

static int
hwloc_solaris_set_sth_cpubind(hwloc_topology_t topology, idtype_t idtype, id_t id, hwloc_cpuset_t hwloc_set, int strict)
{
  unsigned target;

  if (hwloc_cpuset_isequal(hwloc_set, hwloc_get_system_obj(topology)->cpuset)) {
    if (processor_bind(idtype, id, PBIND_NONE, NULL) != 0)
      return -1;
    return 0;
  }

  if (hwloc_cpuset_weight(hwloc_set) != 1) {
    errno = EXDEV;
    return -1;
  }

  target = hwloc_cpuset_first(hwloc_set);

  if (processor_bind(idtype, id,
		     (processorid_t) (target), NULL) != 0)
    return -1;

  return 0;
}

static int
hwloc_solaris_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_solaris_set_sth_cpubind(topology, P_PID, pid, hwloc_set, strict);
}

static int
hwloc_solaris_set_thisproc_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_solaris_set_sth_cpubind(topology, P_PID, P_MYID, hwloc_set, strict);
}

static int
hwloc_solaris_set_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_solaris_set_thisproc_cpubind(topology, hwloc_set, strict);
}

static int
hwloc_solaris_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_cpuset_t hwloc_set, int strict)
{
  return hwloc_solaris_set_sth_cpubind(topology, P_LWPID, P_MYID, hwloc_set, strict);
}

/* TODO: thread, maybe not easy because of the historical n:m implementation */

#ifdef HAVE_LIBLGRP
#      include <sys/lgrp_user.h>
static void
browse(struct hwloc_topology *topology, lgrp_cookie_t cookie, lgrp_id_t lgrp, hwloc_obj_t *glob_lgrps, unsigned *curlgrp)
{
  int n;
  hwloc_obj_t obj;
  lgrp_mem_size_t mem_size;

  n = lgrp_cpus(cookie, lgrp, NULL, 0, LGRP_CONTENT_ALL);
  if (n == -1)
    return;

  if ((mem_size = lgrp_mem_size(cookie, lgrp, LGRP_MEM_SZ_INSTALLED, LGRP_CONTENT_DIRECT)) > 0)
  {
    int i;
    processorid_t cpuids[n];

    obj = hwloc_alloc_setup_object(HWLOC_OBJ_NODE, lgrp);
    obj->cpuset = hwloc_cpuset_alloc();
    glob_lgrps[(*curlgrp)++] = obj;

    lgrp_cpus(cookie, lgrp, cpuids, n, LGRP_CONTENT_ALL);
    for (i = 0; i < n ; i++) {
      hwloc_debug("node %ld's cpu %d is %d\n", lgrp, i, cpuids[i]);
      hwloc_cpuset_set(obj->cpuset, cpuids[i]);
    }
    hwloc_debug_1arg_cpuset("node %ld has cpuset %s\n",
	lgrp, obj->cpuset);

    /* or LGRP_MEM_SZ_FREE */
    obj->attr->node.huge_page_free = 0; /* TODO */
    hwloc_debug("node %ld has %lldkB\n", lgrp, mem_size/1024);
    obj->attr->node.memory_kB = mem_size / 1024;
    hwloc_add_object(topology, obj);
  }

  n = lgrp_children(cookie, lgrp, NULL, 0);
  {
    lgrp_id_t lgrps[n];
    int i;

    lgrp_children(cookie, lgrp, lgrps, n);
    hwloc_debug("lgrp %ld has %d children\n", lgrp, n);
    for (i = 0; i < n ; i++)
      {
	browse(topology, cookie, lgrps[i], glob_lgrps, curlgrp);
      }
    hwloc_debug("lgrp %ld's children done\n", lgrp);
  }
}

static void
hwloc_look_lgrp(struct hwloc_topology *topology)
{
  lgrp_cookie_t cookie;
  unsigned curlgrp = 0;
  int nlgrps;

  if ((topology->flags & HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM))
    cookie = lgrp_init(LGRP_VIEW_OS);
  else
    cookie = lgrp_init(LGRP_VIEW_CALLER);
  lgrp_id_t root;
  if (cookie == LGRP_COOKIE_NONE)
    {
      hwloc_debug("lgrp_init failed: %s\n", strerror(errno));
      return;
    }
  nlgrps = lgrp_nlgrps(cookie);
  root = lgrp_root(cookie);
  {
    hwloc_obj_t glob_lgrps[nlgrps];
    browse(topology, cookie, root, glob_lgrps, &curlgrp);
    {
      unsigned distances[curlgrp][curlgrp];
      unsigned i, j;
      for (i = 0; i < curlgrp; i++)
	for (j = 0; j < curlgrp; j++)
	  distances[i][j] = lgrp_latency_cookie(cookie, glob_lgrps[i]->os_index, glob_lgrps[j]->os_index, LGRP_LAT_CPU_TO_MEM);
      hwloc_setup_misc_level_from_distances(topology, curlgrp, glob_lgrps, (unsigned*) distances);
    }
  }
  lgrp_fini(cookie);
}
#endif /* LIBLGRP */

#ifdef HAVE_LIBKSTAT
#include <kstat.h>
static void
hwloc_look_kstat(struct hwloc_topology *topology, unsigned *nbprocs, hwloc_cpuset_t online_cpuset)
{
  kstat_ctl_t *kc = kstat_open();
  kstat_t *ksp;
  kstat_named_t *stat;
  unsigned look_cores = 1, look_chips = 1;

  unsigned proc_physids[HWLOC_NBMAXCPUS];
  unsigned proc_osphysids[HWLOC_NBMAXCPUS];
  unsigned osphysids[HWLOC_NBMAXCPUS];

  unsigned proc_coreids[HWLOC_NBMAXCPUS];
  unsigned proc_oscoreids[HWLOC_NBMAXCPUS];
  unsigned oscoreids[HWLOC_NBMAXCPUS];

  unsigned core_osphysids[HWLOC_NBMAXCPUS];

  unsigned physid, coreid, cpuid;
  unsigned procid_max = 0;
  unsigned numsockets = 0;
  unsigned numcores = 0;
  unsigned i;

  if (!kc)
    {
      hwloc_debug("kstat_open failed: %s\n", strerror(errno));
      return;
    }

  hwloc_cpuset_zero(online_cpuset);
  for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next)
    {
      if (strncmp("cpu_info", ksp->ks_module, 8))
	continue;

      cpuid = ksp->ks_instance;
      if (cpuid > HWLOC_NBMAXCPUS)
	{
	  fprintf(stderr,"CPU id too big: %u\n", cpuid);
	  continue;
	}

      proc_physids[cpuid] = -1;
      proc_osphysids[cpuid] = -1;
      proc_coreids[cpuid] = -1;
      proc_oscoreids[cpuid] = -1;

      if (kstat_read(kc, ksp, NULL) == -1)
	{
	  fprintf(stderr, "kstat_read failed for CPU%u: %s\n", cpuid, strerror(errno));
	  goto out;
	}

      stat = (kstat_named_t *) kstat_data_lookup(ksp, "state");
      if (!stat)
	{
	  hwloc_debug("could not read state for CPU%u: %s\n", cpuid, strerror(errno));
	  continue;
	}
      if (stat->data_type != KSTAT_DATA_CHAR)
	{
	  hwloc_debug("unknown kstat type %d for cpu state\n", stat->data_type);
	  continue;
	}

      procid_max++;
      hwloc_debug("cpu%d's state is %s\n", cpuid, stat->value.c);
      if (strcmp(stat->value.c, "on-line"))
	/* not online, ignore for chip and core ids */
	continue;

      hwloc_cpuset_set(online_cpuset, cpuid);

      if (look_chips) do {
	/* Get Chip ID */
	stat = (kstat_named_t *) kstat_data_lookup(ksp, "chip_id");
	if (!stat)
	  {
	    if (numsockets)
	      fprintf(stderr, "could not read socket id for CPU%u: %s\n", cpuid, strerror(errno));
	    else
	      hwloc_debug("could not read socket id for CPU%u: %s\n", cpuid, strerror(errno));
	    look_chips = 0;
	    continue;
	  }
	switch (stat->data_type) {
	  case KSTAT_DATA_INT32:
	    physid = stat->value.i32;
	    break;
	  case KSTAT_DATA_UINT32:
	    physid = stat->value.ui32;
	    break;
#ifdef _INT64_TYPE
	  case KSTAT_DATA_UINT64:
	    physid = stat->value.ui64;
	    break;
	  case KSTAT_DATA_INT64:
	    physid = stat->value.i64;
	    break;
#endif
	  default:
	    fprintf(stderr, "chip_id type %d unknown\n", stat->data_type);
	    look_chips = 0;
	    continue;
	}
	proc_osphysids[cpuid] = physid;
	for (i = 0; i < numsockets; i++)
	  if (physid == osphysids[i])
	    break;
	proc_physids[cpuid] = i;
	hwloc_debug("%u on socket %u (%u)\n", cpuid, i, physid);
	if (i == numsockets)
	  osphysids[numsockets++] = physid;
      } while(0);

      if (look_cores) do {
	/* Get Core ID */
	stat = (kstat_named_t *) kstat_data_lookup(ksp, "core_id");
	if (!stat)
	  {
	    if (numcores)
	      fprintf(stderr, "could not read core id for CPU%u: %s\n", cpuid, strerror(errno));
	    else
	      hwloc_debug("could not read core id for CPU%u: %s\n", cpuid, strerror(errno));
	    look_cores = 0;
	    continue;
	  }
	switch (stat->data_type) {
	  case KSTAT_DATA_INT32:
	    coreid = stat->value.i32;
	    break;
	  case KSTAT_DATA_UINT32:
	    coreid = stat->value.ui32;
	    break;
#ifdef _INT64_TYPE
	  case KSTAT_DATA_UINT64:
	    coreid = stat->value.ui64;
	    break;
	  case KSTAT_DATA_INT64:
	    coreid = stat->value.i64;
	    break;
#endif
	  default:
	    fprintf(stderr, "core_id type %d unknown\n", stat->data_type);
	    look_cores = 0;
	    continue;
	}
	proc_oscoreids[cpuid] = coreid;
	for (i = 0; i < numcores; i++)
	  if (coreid == oscoreids[i] && proc_osphysids[cpuid] == core_osphysids[i])
	    break;
	proc_coreids[cpuid] = i;
	hwloc_debug("%u on core %u (%u)\n", cpuid, i, coreid);
	if (i == numcores)
	  {
	    core_osphysids[numcores] = proc_osphysids[cpuid];
	    oscoreids[numcores++] = coreid;
	  }
      } while(0);

      /* Note: there is also clog_id for the Thread ID (not unique) and
       * pkg_core_id for the core ID (not unique).  They are not useful to us
       * however. */
    }

  *nbprocs = hwloc_cpuset_weight(online_cpuset);

  if (look_chips)
    hwloc_setup_level(procid_max, numsockets, osphysids, proc_physids, topology, HWLOC_OBJ_SOCKET);

  if (look_cores)
    hwloc_setup_level(procid_max, numcores, oscoreids, proc_coreids, topology, HWLOC_OBJ_CORE);

 out:
  kstat_close(kc);
}
#endif /* LIBKSTAT */

void
hwloc_look_solaris(struct hwloc_topology *topology)
{
  hwloc_cpuset_t online_cpuset = hwloc_cpuset_alloc();
  unsigned nbprocs = hwloc_fallback_nbprocessors ();
#ifdef HAVE_LIBLGRP
  hwloc_look_lgrp(topology);
#endif /* HAVE_LIBLGRP */
  hwloc_cpuset_fill(online_cpuset);
#ifdef HAVE_LIBKSTAT
  hwloc_look_kstat(topology, &nbprocs, online_cpuset);
#endif /* HAVE_LIBKSTAT */
  hwloc_setup_proc_level(topology, nbprocs, online_cpuset);
  free(online_cpuset);
}

void
hwloc_set_solaris_hooks(struct hwloc_topology *topology)
{
  topology->set_cpubind = hwloc_solaris_set_cpubind;
  topology->set_proc_cpubind = hwloc_solaris_set_proc_cpubind;
  topology->set_thisproc_cpubind = hwloc_solaris_set_thisproc_cpubind;
  topology->set_thisthread_cpubind = hwloc_solaris_set_thisthread_cpubind;
}

/* TODO:
 * memory binding: lgrp_affinity_set
 * madvise(MADV_ACCESS_LWP / ACCESS_MANY)
 */
