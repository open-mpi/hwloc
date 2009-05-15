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

#include <topology.h>
#include <topology/helper.h>
#include <topology/debug.h>

#include <numa.h>

void
look_osf(struct topo_topology *topology)
{
  cpu_cursor_t cursor;
  unsigned i = 0;
  unsigned nbnodes;
  radid_t radid;
  cpuid_t cpuid;
  cpuset_t cpuset;
  struct topo_obj **node_level;

  nbnodes=rad_get_num();

  node_level = calloc(nbnodes+1, sizeof(*node_level));

  cpusetcreate(&cpuset);
  for (radid = 0; radid < nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }

    node_level[i] = malloc(sizeof(struct topo_obj));
    assert(node_level[i]);
    topo_setup_object(node_level[i], TOPO_OBJ_NODE, radid);
    node_level[i]->memory_kB = 0; /* TODO */
    node_level[i]->huge_page_free = 0;

    cursor = SET_CURSOR_INIT;
    while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
      topo_cpuset_set(&node_level[i]->cpuset,cpuid);

    topo_debug("node %d has cpuset %"TOPO_PRIxCPUSET"\n",
	       i, TOPO_CPUSET_PRINTF_VALUE(node_level[i]->cpuset));
    i++;
  }

  topology->level_nbitems[topology->nb_levels] = nbnodes;
  topology->levels[topology->nb_levels++] = node_level;

  /* we have a contigous range of online cpus */
  topo_cpuset_set_range(&topology->online_cpuset, 0, topology->nb_processors-1);
}
