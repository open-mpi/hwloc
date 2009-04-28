/* Copyright 2009 INRIA, Université Bordeaux 1  */

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

#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#include <sys/rset.h>

static void
look_rset(int sdl, enum lt_level_e level, struct topo_topology *topology)
{
  rsethandle_t rset, rad;
  int r,i,maxcpus,j;
  unsigned nbnodes;
  struct lt_level *rad_level;

  rset = rs_alloc(RS_PARTITION);
  rad = rs_alloc(RS_EMPTY);
  nbnodes = rs_numrads(rset, sdl, 0);
  if (nbnodes == -1) {
    perror("rs_numrads");
    return;
  }
  if (nbnodes == 1)
    return;

  rad_level=malloc((nbnodes+1)*sizeof(*rad_level));

  for (r = 0, i = 0; i < nbnodes; i++) {
    if (rs_getrad(rset, rad, sdl, i, 0)) {
      fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
      continue;
    }
    if (!rs_getinfo(rad, R_NUMPROCS, 0))
      continue;

    lt_setup_level(&rad_level[r], level);
    lt_set_os_numbers(&rad_level[r], level, r);
    switch(level) {
      case LT_LEVEL_NODE:
	rad_level[r].memory_kB[LT_LEVEL_MEMORY_NODE] = 0; /* TODO */
	rad_level[r].huge_page_free = 0;
	break;
      case LT_LEVEL_L2:
	rad_level[r].memory_kB[LT_LEVEL_MEMORY_L2] = 0; /* TODO */
	break;
      default:
	break;
    }
    maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
    for (j = 0; j < maxcpus; j++) {
      if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
	lt_cpuset_set(&rad_level[r].cpuset,j);
    }
    ltdebug("node %d has cpuset %"LT_PRIxCPUSET"\n",
	   r, LT_CPUSET_PRINTF_VALUE(rad_level[r].cpuset));
    r++;
  }

  lt_cpuset_zero(&rad_level[r].cpuset);

  topology->level_nbitems[topology->nb_levels] = nbnodes;
  topology->levels[topology->nb_levels++] = rad_level;
  rs_free(rset);
  rs_free(rad);
}

void
look_aix(struct topo_topology *topology)
{
  unsigned i;
  for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++)
    {
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  ltdebug("looking AIX node sdl %d\n",i);
	  look_rset(i, LT_LEVEL_NODE, topology);
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  ltdebug("looking AIX L2 sdl %d\n",i);
	  look_rset(i, LT_LEVEL_L2, topology);
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  ltdebug("looking AIX core sdl %d\n",i);
	  look_rset(i, LT_LEVEL_CORE, topology);
	}
#      endif
      if (i == rs_getinfo(NULL, R_SMPSDL, 0))
	ltdebug("not looking AIX \"SMP\" sdl %d\n",i);
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	{
	  ltdebug("looking AIX max sdl %d\n",i);
	  look_rset(i, LT_LEVEL_PROC, topology);
	}
    }
}
