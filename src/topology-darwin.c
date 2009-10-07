/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/* Detect topology change: registering for power management changes and check
 * if for example hw.activecpu changed */

#include <private/config.h>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <inttypes.h>

#include <hwloc.h>
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
hwloc_look_darwin(struct hwloc_topology *topology)
{
  int nprocs;
  int npackages;
  int i, j, cpu;
  struct hwloc_obj *obj;
  size_t size;

  if (get_sysctl("hw.ncpu", &nprocs))
    return;

  hwloc_debug("%d procs\n", nprocs);

  if (!get_sysctl("hw.packages", &npackages)) {
    int cores_per_package;
    int logical_per_package;

    hwloc_debug("%d packages\n", npackages);

    if (get_sysctl("machdep.cpu.logical_per_package", &logical_per_package))
      /* Assume the trivia.  */
      logical_per_package = nprocs / npackages;

    hwloc_debug("%d threads per package\n", logical_per_package);

    assert(nprocs == npackages * logical_per_package);

    for (i = 0; i < npackages; i++) {
      obj = hwloc_alloc_setup_object(HWLOC_OBJ_SOCKET, i);
      obj->cpuset = hwloc_cpuset_alloc();
      for (cpu = i*logical_per_package; cpu < (i+1)*logical_per_package; cpu++)
	hwloc_cpuset_set(obj->cpuset, cpu);

      hwloc_debug_1arg_cpuset("package %d has cpuset %s\n",
		 i, obj->cpuset);
      hwloc_add_object(topology, obj);
    }

    if (!get_sysctl("machdep.cpu.cores_per_package", &cores_per_package)) {
      hwloc_debug("%d cores per package\n", cores_per_package);

      assert(!(logical_per_package % cores_per_package));

      for (i = 0; i < npackages * cores_per_package; i++) {
	obj = hwloc_alloc_setup_object(HWLOC_OBJ_CORE, i);
	obj->cpuset = hwloc_cpuset_alloc();
	for (cpu = i*(logical_per_package/cores_per_package);
	     cpu < (i+1)*(logical_per_package/cores_per_package);
	     cpu++)
	  hwloc_cpuset_set(obj->cpuset, cpu);

        hwloc_debug_1arg_cpuset("core %d has cpuset %s\n",
		   i, obj->cpuset);
	hwloc_add_object(topology, obj);
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

    hwloc_debug("caches");
    for (i = 0; i < n && cacheconfig[i]; i++)
      hwloc_debug(" %"PRId64"(%"PRId64"kB)", cacheconfig[i], cachesize[i] / 1024);

    /* Now we know how many caches there are */
    n = i;
    hwloc_debug("\n%d cache levels\n", n - 1);

    for (i = 0; i < n; i++) {
      for (j = 0; j < nprocs / cacheconfig[i]; j++) {
	obj = hwloc_alloc_setup_object(i?HWLOC_OBJ_CACHE:HWLOC_OBJ_NODE, j);
	obj->cpuset = hwloc_cpuset_alloc();
	for (cpu = j*cacheconfig[i];
	     cpu < (j+1)*cacheconfig[i];
	     cpu++)
	  hwloc_cpuset_set(obj->cpuset, cpu);

	if (i) {
          hwloc_debug_2args_cpuset("L%dcache %d has cpuset %s\n",
	      i, j, obj->cpuset);
	  obj->attr->cache.depth = i;
	  obj->attr->cache.memory_kB = cachesize[i] / 1024;
	} else {
          hwloc_debug_1arg_cpuset("node %d has cpuset %s\n",
	      j, obj->cpuset);
	  obj->attr->node.memory_kB = cachesize[i] / 1024;
	  obj->attr->node.huge_page_free = 0; /* TODO */
	}

	hwloc_add_object(topology, obj);
      }
    }
  }

  /* add PROC objects */
  hwloc_setup_proc_level(topology, nprocs, NULL);
}

void
hwloc_set_darwin_hooks(struct hwloc_topology *topology)
{
}
