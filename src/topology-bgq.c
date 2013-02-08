/*
 * Copyright © 2013 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>

#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

#include <stdlib.h>
#include <sys/utsname.h>
#include <spi/include/kernel/location.h>

static int
hwloc_look_bgq(struct hwloc_backend *backend)
{
  struct hwloc_topology *topology = backend->topology;
  unsigned i;

  if (!topology->levels[0][0]->cpuset) {
    /* Nobody created objects yet, setup everything */
    hwloc_bitmap_t set;
    hwloc_obj_t obj;

#define HWLOC_BGQ_CORES 17 /* spare core ignored for now */

    hwloc_alloc_obj_cpusets(topology->levels[0][0]);
    /* mark the 17th core (OS-reserved) as disallowed */
    hwloc_bitmap_clr_range(topology->levels[0][0]->allowed_cpuset, (HWLOC_BGQ_CORES-1)*4, HWLOC_BGQ_CORES*4-1);

    /* a single memory bank */
    set = hwloc_bitmap_alloc();
    hwloc_bitmap_set(set, 0);
    topology->levels[0][0]->nodeset = set;
    topology->levels[0][0]->memory.local_memory = 16ULL*1024*1024*1024ULL;

    /* socket */
    obj = hwloc_alloc_setup_object(HWLOC_OBJ_SOCKET, 0);
    set = hwloc_bitmap_alloc();
    hwloc_bitmap_set_range(set, 0, HWLOC_BGQ_CORES*4-1);
    obj->cpuset = set;
    hwloc_obj_add_info(obj, "CPUModel", "IBM PowerPC A2");
    hwloc_insert_object_by_cpuset(topology, obj);

    /* shared L2 */
    obj = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, -1);
    obj->cpuset = hwloc_bitmap_dup(set);
    obj->attr->cache.type = HWLOC_OBJ_CACHE_UNIFIED;
    obj->attr->cache.depth = 2;
    obj->attr->cache.size = 32*1024*1024;
    obj->attr->cache.linesize = 128;
    obj->attr->cache.associativity = 16;
    hwloc_insert_object_by_cpuset(topology, obj);

    /* Cores */
    for(i=0; i<HWLOC_BGQ_CORES; i++) {
      /* Core */
      obj = hwloc_alloc_setup_object(HWLOC_OBJ_CORE, i);
      set = hwloc_bitmap_alloc();
      hwloc_bitmap_set_range(set, i*4, i*4+3);
      obj->cpuset = set;
      hwloc_insert_object_by_cpuset(topology, obj);
      /* L1d */
      obj = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, -1);
      obj->cpuset = hwloc_bitmap_dup(set);
      obj->attr->cache.type = HWLOC_OBJ_CACHE_DATA;
      obj->attr->cache.depth = 1;
      obj->attr->cache.size = 16*1024;
      obj->attr->cache.linesize = 64;
      obj->attr->cache.associativity = 8;
      hwloc_insert_object_by_cpuset(topology, obj);
      /* L1i */
      obj = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, -1);
      obj->cpuset = hwloc_bitmap_dup(set);
      obj->attr->cache.type = HWLOC_OBJ_CACHE_INSTRUCTION;
      obj->attr->cache.depth = 1;
      obj->attr->cache.size = 16*1024;
      obj->attr->cache.linesize = 64;
      obj->attr->cache.associativity = 4;
      hwloc_insert_object_by_cpuset(topology, obj);
      /* there's also a L1p "prefetch cache" of 4kB with 128B lines */
    }

    /* PUs */
    hwloc_setup_pu_level(topology, HWLOC_BGQ_CORES*4);
  }

  /* Add BGQ specific information */

  hwloc_obj_add_info(topology->levels[0][0], "Backend", "BGQ");
  if (topology->is_thissystem)
    hwloc_add_uname_info(topology);
  return 1;
}

static int
hwloc_bgq_get_thisthread_cpubind(hwloc_topology_t topology, hwloc_bitmap_t hwloc_set, int flags __hwloc_attribute_unused)
{
  if (topology->pid) {
    errno = ENOSYS;
    return -1;
  }
  hwloc_bitmap_only(hwloc_set, Kernel_ProcessorID());
  return 0;
}

void
hwloc_set_bgq_hooks(struct hwloc_binding_hooks *hooks __hwloc_attribute_unused,
		    struct hwloc_topology_support *support __hwloc_attribute_unused)
{
  hooks->get_thisthread_cpubind = hwloc_bgq_get_thisthread_cpubind;
  hooks->get_thisthread_last_cpu_location = hwloc_bgq_get_thisthread_cpubind;
}

static struct hwloc_backend *
hwloc_bgq_component_instantiate(struct hwloc_disc_component *component,
				const void *_data1 __hwloc_attribute_unused,
				const void *_data2 __hwloc_attribute_unused,
				const void *_data3 __hwloc_attribute_unused)
{
  struct utsname utsname;
  struct hwloc_backend *backend;
  char *env;
  int err;

  env = getenv("HWLOC_FORCE_BGQ");
  if (!env || !atoi(env)) {
    err = uname(&utsname);
    if (err || strcmp(utsname.sysname, "CNK") || strcmp(utsname.machine, "BGQ")) {
      fprintf(stderr, "*** Found unexpected uname sysname `%s' machine `%s', disabling BGQ backend.\n", utsname.sysname, utsname.machine);
      fprintf(stderr, "*** Set HWLOC_FORCE_BGQ=1 in the environment to enforce the BGQ backend.\n");
      return NULL;
    }
  }

  backend = hwloc_backend_alloc(component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_look_bgq;
  return backend;
}

static struct hwloc_disc_component hwloc_bgq_disc_component = {
  HWLOC_DISC_COMPONENT_TYPE_CPU,
  "bgq",
  HWLOC_DISC_COMPONENT_TYPE_GLOBAL,
  hwloc_bgq_component_instantiate,
  50,
  NULL
};

const struct hwloc_component hwloc_bgq_component = {
  HWLOC_COMPONENT_ABI,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_bgq_disc_component
};
