/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <inttypes.h>

#include <topology.h>
#include <private/private.h>
#include <private/debug.h>

static int get_sysctl(const char *name, int *res)
{
  int n;
  size_t size = sizeof(n);
  if (sysctlbyname(name, &n, &size, NULL, 0))
    return -1;
  if (size != sizeof(n))
    return -1;
  *res = n;
  return 0;
}

void
topo_look_darwin(struct topo_topology *topology)
{
  int nprocs;
  int npackages;
  int i, j, cpu;
  struct topo_obj *obj;
  size_t size;

  if (get_sysctl("hw.ncpu", &nprocs))
    return;

  topo_debug("%d procs\n", nprocs);

  if (!get_sysctl("hw.packages", &npackages)) {
    int cores_per_package;
    int logical_per_package;

    topo_debug("%d packages\n", npackages);

    if (get_sysctl("machdep.cpu.logical_per_package", &logical_per_package))
      /* Assume the trivia.  */
      logical_per_package = nprocs / npackages;

    topo_debug("%d threads per package\n", logical_per_package);

    assert(nprocs == npackages * logical_per_package);

    for (i = 0; i < npackages; i++) {
      obj = topo_alloc_setup_object(TOPO_OBJ_SOCKET, i);
      for (cpu = i*logical_per_package; cpu < (i+1)*logical_per_package; cpu++)
	topo_cpuset_set(&obj->cpuset, cpu);

      topo_debug("package %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 i, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
      topo_add_object(topology, obj);
    }

    if (!get_sysctl("machdep.cpu.cores_per_package", &cores_per_package)) {
      topo_debug("%d cores per package\n", cores_per_package);

      assert(!(logical_per_package % cores_per_package));

      for (i = 0; i < npackages * cores_per_package; i++) {
	obj = topo_alloc_setup_object(TOPO_OBJ_CORE, i);
	for (cpu = i*(logical_per_package/cores_per_package);
	     cpu < (i+1)*(logical_per_package/cores_per_package);
	     cpu++)
	  topo_cpuset_set(&obj->cpuset, cpu);

	topo_debug("core %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   i, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
	topo_add_object(topology, obj);
      }
    }
  }

  if (!sysctlbyname("hw.cacheconfig", NULL, &size, NULL, 0)) {
    int n = size / sizeof(uint64_t);
    uint64_t cacheconfig[n];
    uint64_t cachesize[n];

    assert(!sysctlbyname("hw.cacheconfig", cacheconfig, &size, NULL, 0));

    memset(cachesize, 0, sizeof(cachesize));
    size = sizeof(cachesize);
    sysctlbyname("hw.cachesize", cachesize, &size, NULL, 0);

    topo_debug("caches");
    for (i = 0; i < n && cacheconfig[i]; i++)
      topo_debug(" %"PRId64"(%"PRId64"kB)", cacheconfig[i], cachesize[i] / 1024);

    /* Now we know how many caches there are */
    n = i;
    topo_debug("\n%d cache levels\n", n - 1);

    for (i = 0; i < n; i++) {
      for (j = 0; j < nprocs / cacheconfig[i]; j++) {
	obj = topo_alloc_setup_object(i?TOPO_OBJ_CACHE:TOPO_OBJ_NODE, j);
	for (cpu = j*cacheconfig[i];
	     cpu < (j+1)*cacheconfig[i];
	     cpu++)
	  topo_cpuset_set(&obj->cpuset, cpu);

	if (i) {
	  topo_debug("L%dcache %d has cpuset %"TOPO_PRIxCPUSET"\n",
	      i, j, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
	  obj->attr->cache.depth = i;
	  obj->attr->cache.memory_kB = cachesize[i] / 1024;
	} else {
	  topo_debug("node %d has cpuset %"TOPO_PRIxCPUSET"\n",
	      j, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
	  obj->attr->node.memory_kB = cachesize[i] / 1024;
	  obj->attr->node.huge_page_free = 0; /* TODO */
	}

	topo_add_object(topology, obj);
      }
    }
  }

  /* add PROC objects */
  topo_setup_proc_level(topology, nprocs, NULL);
}
