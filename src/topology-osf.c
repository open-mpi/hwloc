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
  struct topo_level *node_level;

  nbnodes=rad_get_num();
  if (nbnodes==1)
    return;

  node_level=malloc((nbnodes+1)*sizeof(*node_level));

  cpusetcreate(&cpuset);
  for (radid = 0; radid < nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }

    lt_setup_level(&node_level[i], TOPO_LEVEL_NODE);
    lt_set_os_numbers(&node_level[i], node, radid);
    node_level[i].memory_kB[TOPO_LEVEL_MEMORY_NODE] = 0; /* TODO */
    node_level[i].huge_page_free = 0;

    cursor = SET_CURSOR_INIT;
    while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
      topo_cpuset_set(&node_level[i].cpuset,cpuid);

    ltdebug("node %d has cpuset %"LT_PRIxCPUSET"\n",
	   i, TOPO_CPUSET_PRINTF_VALUE(node_level[i].cpuset));
    i++;
  }

  topo_cpuset_zero(&node_level[i].cpuset);

  topology->level_nbitems[topology->nb_levels] = nbnodes;
  topology->levels[topology->nb_levels++] = node_level;
}
