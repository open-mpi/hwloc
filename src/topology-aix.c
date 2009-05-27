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
#include <topology/helper.h>
#include <topology/debug.h>

#include <sys/rset.h>

static void
look_rset(int sdl, enum topo_obj_type_e type, struct topo_topology *topology)
{
  rsethandle_t rset, rad;
  int r,i,maxcpus,j;
  unsigned nbnodes;
  struct topo_obj **rad_level;

  if ((topology->flags & TOPO_FLAGS_IGNORE_ADMIN_DISABLE))
    rset = rs_alloc(RS_ALL);
  else
    rset = rs_alloc(RS_PARTITION);
  rad = rs_alloc(RS_EMPTY);
  nbnodes = rs_numrads(rset, sdl, 0);
  if (nbnodes == -1) {
    perror("rs_numrads");
    return;
  }

  rad_level = calloc(nbnodes+1, sizeof(*rad_level));

  for (r = 0, i = 0; i < nbnodes; i++) {
    if (rs_getrad(rset, rad, sdl, i, 0)) {
      fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
      continue;
    }
    if (!rs_getinfo(rad, R_NUMPROCS, 0))
      continue;

    rad_level[r] = malloc(sizeof(struct topo_obj));
    assert(rad_level[r]);
    topo_setup_object(rad_level[r], type, r);
    switch(type) {
      case TOPO_OBJ_NODE:
	rad_level[r]->memory_kB = 0; /* TODO */
	rad_level[r]->huge_page_free = 0;
	break;
      case TOPO_OBJ_CACHE:
	rad_level[r]->memory_kB = 0; /* TODO */
	rad_level[r]->cache_depth = 2;
	break;
      default:
	break;
    }
    maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
    for (j = 0; j < maxcpus; j++) {
      if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
	topo_cpuset_set(&rad_level[r]->cpuset,j);
    }
    topo_debug("node %d has cpuset %"TOPO_PRIxCPUSET"\n",
	       r, TOPO_CPUSET_PRINTF_VALUE(rad_level[r]->cpuset));
    r++;
  }

  topo_add_level(topology, rad_level, nbnodes);
  rs_free(rset);
  rs_free(rad);
}

void
topo_look_aix(struct topo_topology *topology)
{
  unsigned i;
    /* TODO: R_LGPGDEF/R_LGPGFREE */

  for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++)
    {
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  topo_debug("looking AIX node sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_NODE, topology);
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  topo_debug("looking AIX L2 sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_CACHE, topology);
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  topo_debug("looking AIX core sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_CORE, topology);
	}
#      endif
      if (i == rs_getinfo(NULL, R_SMPSDL, 0))
	topo_debug("not looking AIX \"SMP\" sdl %d\n", i);
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	topo_debug("not looking AIX max sdl %d\n", i);
#if 0
      /* Disabled for now until the generic code accepts it.
       */
	{
	  topo_debug("looking AIX max sdl %d\n", i);
	  look_rset(i, TOPO_OBJ_PROC, topology);
	}
#endif
    }
}
