/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_LIBLGRP
#      include <sys/lgrp_user.h>
static void
show(lgrp_cookie_t cookie, lgrp_id_t lgrp)
{
  processorid_t cpuids[32];
  lgrp_id_t lgrps[32];
  int i, n;
  n = lgrp_cpus(cookie, lgrp, cpuids, 32, LGRP_CONTENT_ALL);
  for (i = 0; i < n ; i++)
    {
      topo_debug("%ld has %d\n", lgrp, cpuids[i]);
    }
  n = lgrp_children(cookie, lgrp, lgrps, 32);
  topo_debug("%ld %d children\n", lgrp, n);
  for (i = 0; i < n ; i++)
    {
      show(cookie, lgrps[i]);
    }
  topo_debug("%ld children done\n", lgrp);
}

void
look_lgrp(struct topo_topology *topology)
{
  lgrp_cookie_t cookie = lgrp_init(LGRP_VIEW_OS);
  lgrp_id_t root;
  if (cookie == LGRP_COOKIE_NONE)
    {
      topo_debug("lgrp_init failed: %s\n", strerror(errno));
      return;
    }
  root = lgrp_root(cookie);
  show(cookie, root);
  /* TODO */
  lgrp_fini(cookie);
}
#endif /* LIBLGRP */

#ifdef HAVE_LIBKSTAT
#include <kstat.h>
void
look_kstat(struct topo_topology *topology)
{
  kstat_ctl_t *kc = kstat_open();
  kstat_t *ksp;
  kstat_named_t *stat;
  unsigned proc_physids[TOPO_NBMAXCPUS];
  unsigned proc_osphysids[TOPO_NBMAXCPUS];
  unsigned osphysids[TOPO_NBMAXCPUS];
  unsigned proc_coreids[TOPO_NBMAXCPUS];
  unsigned proc_oscoreids[TOPO_NBMAXCPUS];
  unsigned oscoreids[TOPO_NBMAXCPUS];
  unsigned core_osphysids[TOPO_NBMAXCPUS];
  unsigned physid, coreid;
  unsigned numprocs = 0;
  unsigned numdies = 0;
  unsigned numcores = 0;
  unsigned i;

  if (!kc)
    {
      topo_debug("kstat_open failed: %s\n", strerror(errno));
      return;
    }
  for (ksp = kc->kc_chain; ksp; ksp = ksp->ks_next)
    {
      if (strncmp("cpu_info", ksp->ks_module, 8))
	continue;

      if (ksp->ks_instance != numprocs)
	{
	  fprintf(stderr, "kstat instances not in CPU order: %d comes %d\n", ksp->ks_instance, numprocs);
	  goto out;
	}
      if (kstat_read(kc, ksp, NULL) == -1)
	{
	  fprintf(stderr, "kstat_read failed for CPU%d: %s\n", numprocs, strerror(errno));
	  goto out;
	}
      stat = (kstat_named_t *) kstat_data_lookup(ksp, "chip_id");
      if (!stat)
	{
	  if (numdies)
	    {
	      fprintf(stderr, "could not read die id for CPU%d: %s\n", numprocs, strerror(errno));
	      goto out;
	    }
	}
      else
	{
	  if (stat->data_type != KSTAT_DATA_INT32)
	    {
	      fprintf(stderr, "chip_id is not an INT32\n");
	      goto out;
	    }
	  proc_osphysids[numprocs] = physid = stat->value.i32;
	  for (i = 0; i < numdies; i++)
	    if (physid == osphysids[i])
	      break;
	  proc_physids[numprocs] = i;
	  topo_debug("%u on die %u (%u)\n", numprocs, i, physid);
	  if (i == numdies)
	    osphysids[numdies++] = physid;
	}

      stat = (kstat_named_t *) kstat_data_lookup(ksp, "core_id");
      if (!stat)
	{
	  if (numcores)
	    {
	      fprintf(stderr, "could not read core id for CPU%d: %s\n", numprocs, strerror(errno));
	      goto out;
	    }
	}
      else
	{
	  if (stat->data_type != KSTAT_DATA_INT32)
	    {
	      fprintf(stderr, "core_id is not an INT32\n");
	      goto out;
	    }
	  proc_oscoreids[numprocs] = coreid = stat->value.i32;
	  for (i = 0; i < numcores; i++)
	    if (coreid == oscoreids[i] && proc_osphysids[numprocs] == core_osphysids[i])
	      break;
	  proc_coreids[numprocs] = i;
	  topo_debug("%u on core %u (%u)\n", numprocs, i, coreid);
	  if (i == numcores)
	    {
	      core_osphysids[numcores] = proc_osphysids[numprocs];
	      oscoreids[numcores++] = coreid;
	    }
	}

      numprocs++;
    }

  if (numdies > 1)
    topo_setup_die_level(numprocs, numdies, osphysids, proc_physids, topology);

  if (numcores > 1)
    topo_setup_core_level(numprocs, numcores, oscoreids, proc_coreids, topology);

  /* we have a contigous range of online cpus */
  topo_cpuset_set_range(&topology->online_cpuset, 0, topology->nb_processors-1);

 out:
  kstat_close(kc);
}
#endif /* LIBKSTAT */

