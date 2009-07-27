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

#include <private/config.h>
#include <topology.h>
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
topo_solaris_set_sth_cpubind(topo_topology_t topology, idtype_t idtype, id_t id, const topo_cpuset_t *topo_set, int strict)
{
  unsigned target;

  if (topo_cpuset_isequal(topo_set, &topo_get_system_obj(topology)->cpuset)) {
    if (processor_bind(idtype, id, PBIND_NONE, NULL) != 0)
      return -1;
    return 0;
  }

  if (topo_cpuset_weight(topo_set) != 1) {
    errno = EXDEV;
    return -1;
  }

  target = topo_cpuset_first(topo_set);

  if (processor_bind(idtype, id,
		     (processorid_t) (target), NULL) != 0)
    return -1;

  return 0;
}

static int
topo_solaris_set_proc_cpubind(topo_topology_t topology, topo_pid_t pid, const topo_cpuset_t *topo_set, int strict)
{
  return topo_solaris_set_sth_cpubind(topology, P_PID, pid, topo_set, strict);
}

static int
topo_solaris_set_thisproc_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_solaris_set_sth_cpubind(topology, P_PID, P_MYID, topo_set, strict);
}

static int
topo_solaris_set_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_solaris_set_thisproc_cpubind(topology, topo_set, strict);
}

static int
topo_solaris_set_thisthread_cpubind(topo_topology_t topology, const topo_cpuset_t *topo_set, int strict)
{
  return topo_solaris_set_sth_cpubind(topology, P_LWPID, P_MYID, topo_set, strict);
}

/* TODO: thread, maybe not easy because of the historical n:m implementation */

#ifdef HAVE_LIBLGRP
#      include <sys/lgrp_user.h>
static void
browse(struct topo_topology *topology, lgrp_cookie_t cookie, lgrp_id_t lgrp, topo_obj_t *glob_lgrps, unsigned *curlgrp)
{
  int n;
  topo_obj_t obj;
  lgrp_mem_size_t mem_size;

  n = lgrp_cpus(cookie, lgrp, NULL, 0, LGRP_CONTENT_ALL);
  if (n == -1)
    return;

  if ((mem_size = lgrp_mem_size(cookie, lgrp, LGRP_MEM_SZ_INSTALLED, LGRP_CONTENT_DIRECT)) > 0)
  {
    int i;
    processorid_t cpuids[n];

    obj = topo_alloc_setup_object(TOPO_OBJ_NODE, lgrp);
    glob_lgrps[(*curlgrp)++] = obj;

    lgrp_cpus(cookie, lgrp, cpuids, n, LGRP_CONTENT_ALL);
    for (i = 0; i < n ; i++) {
      topo_debug("node %ld's cpu %d is %d\n", lgrp, i, cpuids[i]);
      topo_cpuset_set(&obj->cpuset, cpuids[i]);
    }
    topo_debug("node %ld has cpuset %"TOPO_PRIxCPUSET"\n",
	lgrp, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));

    /* or LGRP_MEM_SZ_FREE */
    obj->attr->node.huge_page_free = 0; /* TODO */
    topo_debug("node %ld has %lldkB\n", lgrp, mem_size/1024);
    obj->attr->node.memory_kB = mem_size / 1024;
    topo_add_object(topology, obj);
  }

  n = lgrp_children(cookie, lgrp, NULL, 0);
  {
    lgrp_id_t lgrps[n];
    int i;

    lgrp_children(cookie, lgrp, lgrps, n);
    topo_debug("lgrp %ld has %d children\n", lgrp, n);
    for (i = 0; i < n ; i++)
      {
	browse(topology, cookie, lgrps[i], glob_lgrps, curlgrp);
      }
    topo_debug("lgrp %ld's children done\n", lgrp);
  }
}

static void
topo_look_lgrp(struct topo_topology *topology)
{
  lgrp_cookie_t cookie;
  unsigned curlgrp = 0;
  int nlgrps;

  if ((topology->flags & TOPO_FLAGS_WHOLE_SYSTEM))
    cookie = lgrp_init(LGRP_VIEW_OS);
  else
    cookie = lgrp_init(LGRP_VIEW_CALLER);
  lgrp_id_t root;
  if (cookie == LGRP_COOKIE_NONE)
    {
      topo_debug("lgrp_init failed: %s\n", strerror(errno));
      return;
    }
  nlgrps = lgrp_nlgrps(cookie);
  root = lgrp_root(cookie);
  {
    topo_obj_t glob_lgrps[nlgrps];
    browse(topology, cookie, root, glob_lgrps, &curlgrp);
    {
      unsigned distances[curlgrp][curlgrp];
      unsigned i, j;
      for (i = 0; i < curlgrp; i++)
	for (j = 0; j < curlgrp; j++)
	  distances[i][j] = lgrp_latency_cookie(cookie, glob_lgrps[i]->os_index, glob_lgrps[j]->os_index, LGRP_LAT_CPU_TO_MEM);
      topo_setup_misc_level_from_distances(topology, curlgrp, glob_lgrps, distances);
    }
  }
  lgrp_fini(cookie);
}
#endif /* LIBLGRP */

#ifdef HAVE_LIBKSTAT
#include <kstat.h>
static void
topo_look_kstat(struct topo_topology *topology, unsigned *nbprocs, topo_cpuset_t *online_cpuset)
{
  kstat_ctl_t *kc = kstat_open();
  kstat_t *ksp;
  kstat_named_t *stat;
  unsigned look_cores = 1, look_chips = 1;

  unsigned proc_hasphysid[TOPO_NBMAXCPUS] = {};
  unsigned proc_physids[TOPO_NBMAXCPUS];
  unsigned proc_osphysids[TOPO_NBMAXCPUS];
  unsigned osphysids[TOPO_NBMAXCPUS];

  unsigned proc_hascoreid[TOPO_NBMAXCPUS] = {};
  unsigned proc_coreids[TOPO_NBMAXCPUS];
  unsigned proc_oscoreids[TOPO_NBMAXCPUS];
  unsigned oscoreids[TOPO_NBMAXCPUS];

  unsigned core_osphysids[TOPO_NBMAXCPUS];

  unsigned physid, coreid, cpuid;
  unsigned numprocs = 0;
  unsigned numsockets = 0;
  unsigned numcores = 0;
  unsigned i;

  if (!kc)
    {
      topo_debug("kstat_open failed: %s\n", strerror(errno));
      return;
    }

  topo_cpuset_zero(online_cpuset);
  for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next)
    {
      if (strncmp("cpu_info", ksp->ks_module, 8))
	continue;

      cpuid = ksp->ks_instance;
      if (cpuid > TOPO_NBMAXCPUS)
	{
	  fprintf(stderr,"CPU id too big: %d\n", cpuid);
	  continue;
	}

      if (kstat_read(kc, ksp, NULL) == -1)
	{
	  fprintf(stderr, "kstat_read failed for CPU%u: %s\n", cpuid, strerror(errno));
	  goto out;
	}

      stat = (kstat_named_t *) kstat_data_lookup(ksp, "state");
      if (!stat)
	{
	  topo_debug("could not read state for CPU%u: %s\n", cpuid, strerror(errno));
	  continue;
	}
      if (stat->data_type != KSTAT_DATA_CHAR)
	{
	  topo_debug("unknown kstat type %d for cpu state\n", stat->data_type);
	  continue;
	}
      else
	{
	  topo_debug("cpu%d's state is %s\n", cpuid, stat->value.c);
	  if (strcmp(stat->value.c, "on-line"))
	    /* Not online, ignore */
	    continue;
	}
      topo_cpuset_set(online_cpuset, cpuid);

      if (look_chips) do {
	/* Get Chip ID */
	stat = (kstat_named_t *) kstat_data_lookup(ksp, "chip_id");
	if (!stat)
	  {
	    if (numsockets)
	      fprintf(stderr, "could not read socket id for CPU%u: %s\n", cpuid, strerror(errno));
	    else
	      topo_debug("could not read socket id for CPU%u: %s\n", cpuid, strerror(errno));
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
	topo_debug("%u on socket %u (%u)\n", cpuid, i, physid);
	if (i == numsockets)
	  osphysids[numsockets++] = physid;
	proc_hasphysid[cpuid] = 1;
      } while(0);

      if (look_cores) do {
	/* Get Core ID */
	stat = (kstat_named_t *) kstat_data_lookup(ksp, "core_id");
	if (!stat)
	  {
	    if (numcores)
	      fprintf(stderr, "could not read core id for CPU%u: %s\n", cpuid, strerror(errno));
	    else
	      topo_debug("could not read core id for CPU%u: %s\n", cpuid, strerror(errno));
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
	topo_debug("%u on core %u (%u)\n", cpuid, i, coreid);
	if (i == numcores)
	  {
	    core_osphysids[numcores] = proc_osphysids[cpuid];
	    oscoreids[numcores++] = coreid;
	  }
	proc_hascoreid[cpuid] = 1;
      } while(0);

      /* Note: there is also clog_id for the Thread ID (not unique) and
       * pkg_core_id for the core ID (not unique).  They are not useful to us
       * however. */

      numprocs++;
    }

  *nbprocs = topo_cpuset_weight(&online_cpuset);

  if (look_chips) {
    for (i = 0; i < numprocs; i++)
      if (!proc_hasphysid[i])
	break;
    if (i < numprocs)
      fprintf(stderr,"Sparse instance space, not supported (yet)\n");
    else
      topo_setup_level(numprocs, numsockets, osphysids, proc_physids, topology, TOPO_OBJ_SOCKET);
  }

  if (look_cores) {
    for (i = 0; i < numprocs; i++)
      if (!proc_hascoreid[i])
	break;
    if (i < numprocs)
      fprintf(stderr,"Sparse instance space, not supported (yet)\n");
    else
      topo_setup_level(numprocs, numcores, oscoreids, proc_coreids, topology, TOPO_OBJ_CORE);
  }

 out:
  kstat_close(kc);
}
#endif /* LIBKSTAT */

void topo_look_solaris(struct topo_topology *topology)
{
  topo_cpuset_t online_cpuset;
  unsigned nbprocs = topo_fallback_nbprocessors ();
  topology->set_cpubind = topo_solaris_set_cpubind;
  topology->set_proc_cpubind = topo_solaris_set_proc_cpubind;
  topology->set_thisproc_cpubind = topo_solaris_set_thisproc_cpubind;
  topology->set_thisthread_cpubind = topo_solaris_set_thisthread_cpubind;
#ifdef HAVE_LIBLGRP
  topo_look_lgrp(topology);
#endif /* HAVE_LIBLGRP */
  topo_cpuset_fill(&online_cpuset);
#ifdef HAVE_LIBKSTAT
  topo_look_kstat(topology, &nbprocs, &online_cpuset);
#endif /* HAVE_LIBKSTAT */
  topo_setup_proc_level(topology, nbprocs, &online_cpuset);
}

/* TODO:
 * memory binding: lgrp_affinity_set
 * madvise(MADV_ACCESS_LWP / ACCESS_MANY)
 */
