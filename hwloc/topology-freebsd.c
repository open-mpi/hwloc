/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2019 Inria.  All rights reserved.
 * Copyright © 2009-2010, 2012 Université Bordeaux
 * Copyright © 2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"

#include <sys/types.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/param.h>
#include <pthread.h>
#ifdef HAVE_PTHREAD_NP_H
#include <pthread_np.h>
#endif
#ifdef HAVE_SYS_CPUSET_H
#include <sys/cpuset.h>
#endif
#ifdef HAVE_SYS_DOMAINSET_H
#include <sys/domainset.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#include "hwloc.h"
#include "private/private.h"
#include "private/debug.h"

#if defined(HAVE_SYS_CPUSET_H) && defined(HAVE_CPUSET_SETAFFINITY)
static void
hwloc_freebsd_bsd2hwloc(hwloc_bitmap_t hwloc_cpuset, const cpuset_t *cset)
{
  unsigned cpu;
  hwloc_bitmap_zero(hwloc_cpuset);
  for (cpu = 0; cpu < CPU_SETSIZE; cpu++)
    if (CPU_ISSET(cpu, cset))
      hwloc_bitmap_set(hwloc_cpuset, cpu);
}

static void
hwloc_freebsd_hwloc2bsd(hwloc_const_bitmap_t hwloc_cpuset, cpuset_t *cset)
{
  unsigned cpu;
  CPU_ZERO(cset);
  for (cpu = 0; cpu < CPU_SETSIZE; cpu++)
    if (hwloc_bitmap_isset(hwloc_cpuset, cpu))
      CPU_SET(cpu, cset);
}

static int
hwloc_freebsd_set_sth_affinity(hwloc_topology_t topology __hwloc_attribute_unused, cpulevel_t level, cpuwhich_t which, id_t id, hwloc_const_bitmap_t hwloc_cpuset, int flags __hwloc_attribute_unused)
{
  cpuset_t cset;

  hwloc_freebsd_hwloc2bsd(hwloc_cpuset, &cset);

  if (cpuset_setaffinity(level, which, id, sizeof(cset), &cset))
    return -1;

  return 0;
}

static int
hwloc_freebsd_get_sth_affinity(hwloc_topology_t topology __hwloc_attribute_unused, cpulevel_t level, cpuwhich_t which, id_t id, hwloc_bitmap_t hwloc_cpuset, int flags __hwloc_attribute_unused)
{
  cpuset_t cset;

  if (cpuset_getaffinity(level, which, id, sizeof(cset), &cset))
    return -1;

  hwloc_freebsd_bsd2hwloc(hwloc_cpuset, &cset);
  return 0;
}

static int
hwloc_freebsd_set_thisproc_cpubind(hwloc_topology_t topology, hwloc_const_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_set_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, hwloc_cpuset, flags);
}

static int
hwloc_freebsd_get_thisproc_cpubind(hwloc_topology_t topology, hwloc_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_get_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, hwloc_cpuset, flags);
}

static int
hwloc_freebsd_set_thisthread_cpubind(hwloc_topology_t topology, hwloc_const_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_set_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, hwloc_cpuset, flags);
}

static int
hwloc_freebsd_get_thisthread_cpubind(hwloc_topology_t topology, hwloc_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_get_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, hwloc_cpuset, flags);
}

static int
hwloc_freebsd_set_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_set_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, pid, hwloc_cpuset, flags);
}

static int
hwloc_freebsd_get_proc_cpubind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_bitmap_t hwloc_cpuset, int flags)
{
  return hwloc_freebsd_get_sth_affinity(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, pid, hwloc_cpuset, flags);
}

#ifdef HAVE_SYS_DOMAINSET_H

static void
hwloc_freebsd_bsd2hwloc_membind(hwloc_nodeset_t hwloc_nodeset, const domainset_t *mask)
{
  unsigned node;
  hwloc_bitmap_zero(hwloc_nodeset);
  for (node = 0; node < DOMAINSET_SETSIZE; node++)
    if (CPU_ISSET(node, mask))
      hwloc_bitmap_set(hwloc_nodeset, node);
}

static void
hwloc_freebsd_hwloc2bsd_membind(hwloc_const_nodeset_t hwloc_nodeset, domainset_t *mask)
{
  unsigned node;
  DOMAINSET_ZERO(mask);
  for (node = 0; node < DOMAINSET_SETSIZE; node++)
    if (hwloc_bitmap_isset(hwloc_nodeset, node))
      CPU_SET(node, mask);
}

static int
hwloc_freebsd_set_sth_domain(hwloc_topology_t topology __hwloc_attribute_unused, cpulevel_t level, cpuwhich_t which, id_t id, hwloc_const_nodeset_t hwloc_nodeset, hwloc_membind_policy_t policy)
{
  domainset_t mask;
  int freebsdpolicy;

  hwloc_freebsd_hwloc2bsd_membind(hwloc_nodeset, &mask);

  switch (policy) {
    case HWLOC_MEMBIND_DEFAULT:
    case HWLOC_MEMBIND_FIRSTTOUCH:
      freebsdpolicy = DOMAINSET_POLICY_FIRSTTOUCH;
      break;
    case HWLOC_MEMBIND_BIND:
        freebsdpolicy = DOMAINSET_POLICY_PREFER;
      break;
    case HWLOC_MEMBIND_INTERLEAVE:
      freebsdpolicy = DOMAINSET_POLICY_INTERLEAVE;
      break;
    default:
      errno = ENOSYS;
      return -1;
  }

  if (cpuset_setdomain(level, which, id, sizeof(mask), &mask, freebsdpolicy))
    return -1;

  return 0;
}

static int
hwloc_freebsd_get_sth_domain(hwloc_topology_t topology __hwloc_attribute_unused, cpulevel_t level, cpuwhich_t which, id_t id, hwloc_nodeset_t hwloc_nodeset, hwloc_membind_policy_t *policy)
{
  domainset_t mask;
  int freebsdpolicy;

  if (cpuset_getdomain(level, which, id, sizeof(mask), &mask, &freebsdpolicy))
    return -1;

  hwloc_freebsd_bsd2hwloc_membind(hwloc_nodeset, &mask);
  switch (freebsdpolicy) {
    case DOMAINSET_POLICY_FIRSTTOUCH:
      *policy = HWLOC_MEMBIND_FIRSTTOUCH;
      break;
    case DOMAINSET_POLICY_PREFER:
      *policy = HWLOC_MEMBIND_BIND;
      break;
    case DOMAINSET_POLICY_ROUNDROBIN: /*same as INTERLEAVE with page-granularity*/
    case DOMAINSET_POLICY_INTERLEAVE:
      *policy = HWLOC_MEMBIND_INTERLEAVE;
      break;
    default:
      errno = ENOSYS;
      return -1;
  }
  return 0;
}

static int
hwloc_freebsd_set_thisproc_membind(hwloc_topology_t topology, hwloc_const_nodeset_t hwloc_nodeset, hwloc_membind_policy_t policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_set_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, hwloc_nodeset, policy);
}

static int
hwloc_freebsd_get_thisproc_membind(hwloc_topology_t topology, hwloc_nodeset_t hwloc_nodeset, hwloc_membind_policy_t *policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_get_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, -1, hwloc_nodeset, policy);
}

static int
hwloc_freebsd_set_thisthread_membind(hwloc_topology_t topology, hwloc_const_nodeset_t hwloc_nodeset, hwloc_membind_policy_t policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_set_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, hwloc_nodeset, policy);
}

static int
hwloc_freebsd_get_thisthread_membind(hwloc_topology_t topology, hwloc_nodeset_t hwloc_nodeset, hwloc_membind_policy_t *policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_get_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, hwloc_nodeset, policy);
}

static int
hwloc_freebsd_set_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_const_nodeset_t hwloc_nodeset, hwloc_membind_policy_t policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_set_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, pid, hwloc_nodeset, policy);
}

static int
hwloc_freebsd_get_proc_membind(hwloc_topology_t topology, hwloc_pid_t pid, hwloc_nodeset_t hwloc_nodeset, hwloc_membind_policy_t *policy, int flags __hwloc_attribute_unused)
{
  return hwloc_freebsd_get_sth_domain(topology, CPU_LEVEL_WHICH, CPU_WHICH_PID, pid, hwloc_nodeset, policy);
}
#endif

#ifdef hwloc_thread_t

#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
#pragma weak pthread_setaffinity_np
static int
hwloc_freebsd_set_thread_cpubind(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_thread_t tid, hwloc_const_bitmap_t hwloc_cpuset, int flags __hwloc_attribute_unused)
{
  int err;
  cpuset_t cset;

  if (!pthread_setaffinity_np) {
    errno = ENOSYS;
    return -1;
  }

  hwloc_freebsd_hwloc2bsd(hwloc_cpuset, &cset);

  err = pthread_setaffinity_np(tid, sizeof(cset), &cset);

  if (err) {
    errno = err;
    return -1;
  }

  return 0;
}
#endif

#if HAVE_DECL_PTHREAD_GETAFFINITY_NP
#pragma weak pthread_getaffinity_np
static int
hwloc_freebsd_get_thread_cpubind(hwloc_topology_t topology __hwloc_attribute_unused, hwloc_thread_t tid, hwloc_bitmap_t hwloc_cpuset, int flags __hwloc_attribute_unused)
{
  int err;
  cpuset_t cset;

  if (!pthread_getaffinity_np) {
    errno = ENOSYS;
    return -1;
  }

  err = pthread_getaffinity_np(tid, sizeof(cset), &cset);

  if (err) {
    errno = err;
    return -1;
  }

  hwloc_freebsd_bsd2hwloc(hwloc_cpuset, &cset);
  return 0;
}
#endif
#endif
#endif

#if (defined HAVE_SYSCTL) && (defined HAVE_SYS_SYSCTL_H)
static void
hwloc_freebsd_node_meminfo_info(struct hwloc_topology *topology)
{
       int mib[2] = { CTL_HW, HW_PHYSMEM };
       unsigned long physmem;
       size_t len = sizeof(physmem);
       sysctl(mib, 2, &physmem, &len, NULL, 0);
       topology->machine_memory.local_memory = physmem;
       /* we don't know anything about NUMA nodes in this backend.
        * let another backend or the core move that memory to the right NUMA node */
}
#endif

static int
set_locality_info(hwloc_topology_t topology, int ndomains, hwloc_obj_t *nodes){
  char *locality, *ptr;
  size_t len_locality;
  int missing = 0;
  char nb[12];
  uint64_t *distances = malloc(sizeof(uint64_t) * ndomains * ndomains);
  if(distances == NULL){
    free(nodes);
    return -1;
  }

  if(sysctlbyname("vm.phys_locality", NULL, &len_locality, NULL,	0) == -1){
    free(distances);
    free(nodes);
    return -1;
  }
  locality = malloc(len_locality);
  sysctlbyname("vm.phys_locality", locality, &len_locality, NULL,	0);
  for(int i = 0 ; i < ndomains ; i++){
    sprintf(nb, "%d:", i);
    ptr = strstr(locality, nb);
    ptr = ptr + 2;
    for(int j = 0 ; j < ndomains ; j++){
        int tmp = atoi(ptr);
        if(tmp == -1)
          missing = 1;
        else
          distances[ndomains*i + j] = tmp;
        ptr++;
        ptr = strchr(ptr, ' ');
    }
  }

  if(missing == 1){
    free(nodes);
    free(distances);
  } else
    hwloc_internal_distances_add(topology, "NUMA:FreeBSD", ndomains, nodes, distances, HWLOC_DISTANCES_KIND_FROM_OS|HWLOC_DISTANCES_KIND_MEANS_LATENCY, HWLOC_DISTANCES_ADD_FLAG_GROUP);
  return 0;
}

static hwloc_uint64_t*
get_memory_domain_info(int ndomains){
  char *segs;
  char *ptr;
  size_t len_segs;
  long start = 0;
  long end = 0;
  long domain = 0;
  hwloc_uint64_t *domains_memory = malloc(ndomains * sizeof(hwloc_uint64_t));
  if(domains_memory == NULL)
    return NULL;
  memset(domains_memory, 0, ndomains * sizeof(hwloc_uint64_t));

  if(sysctlbyname("vm.phys_segs", NULL, &len_segs, NULL,	0) == -1){
    free(domains_memory);
    return NULL;
  }
  segs = malloc(len_segs);
  if(!segs){
    free(domains_memory);
    return NULL;
  }

  sysctlbyname("vm.phys_segs", segs, &len_segs, NULL,	0);

  ptr = strstr(segs, "start");
  while(ptr){
    start = strtoul(ptr+11, NULL, 16);
    ptr = strstr(ptr, "end");
    end = strtoul(ptr+11, NULL, 16);
    ptr = strstr(ptr, "domain");
    domain = strtoul(ptr+11, NULL, 10);
    ptr = strstr(ptr, "start");
    domains_memory[domain] += ( end - start );
  }

  free(segs);
  free(ptr);

  return domains_memory;
}

static int
hwloc_look_freebsd_domains(struct hwloc_topology *topology){
    unsigned ndomains, i;
	  size_t len_ndomains;
    int err = 0;
    hwloc_obj_t *nodes;

    len_ndomains = sizeof(ndomains);
    if(sysctlbyname("vm.ndomains", &ndomains, &len_ndomains, NULL,	0) == -1)
      return -1;

    hwloc_uint64_t *domains_memory = get_memory_domain_info(ndomains);
    nodes = malloc(sizeof(hwloc_obj_t) * ndomains);
    if(nodes == NULL || domains_memory == NULL){
      free(domains_memory);
      free(nodes);
      return -1;
    }

    for ( i = 0 ; i < ndomains ; i++ ){
      hwloc_obj_t obj;
      cpuset_t cset;
      CPU_ZERO(&cset);
      obj = hwloc_alloc_setup_object(topology, HWLOC_OBJ_NUMANODE, ( unsigned ) i);
      if(obj != NULL) {
        obj->nodeset = hwloc_bitmap_alloc();
        hwloc_bitmap_set(obj->nodeset, i);

        obj->cpuset = hwloc_bitmap_alloc();
        err = cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_DOMAIN, i, sizeof(cset), &cset);
        hwloc_freebsd_bsd2hwloc(obj->cpuset, &cset);

        obj->attr->numanode.local_memory = domains_memory[i];

        nodes[i] = obj;
        hwloc_insert_object_by_cpuset(topology, obj);
      } else {
        nodes = NULL;
      }
    }
  if(nodes != NULL)
    set_locality_info(topology, ndomains, nodes);

  /*nodes is either free in set_locality_info() or sent to the kernel to set locality info*/
  free(domains_memory);

  topology->support.discovery->numa = 1;
  topology->support.discovery->numa_memory = 1;

  return err;
}

static int
hwloc_look_freebsd(struct hwloc_backend *backend, struct hwloc_disc_status *dstatus)
{
  /*
   * This backend uses the underlying OS.
   * However we don't enforce topology->is_thissystem so that
   * we may still force use this backend when debugging with !thissystem.
   */
  struct hwloc_topology *topology = backend->topology;

  if (dstatus->phase == HWLOC_DISC_PHASE_CPU) {
    if (!topology->levels[0][0]->cpuset) {
      /* Nobody (even the x86 backend) created objects yet, setup basic objects */
      int nbprocs = hwloc_fallback_nbprocessors(0);
      if (nbprocs >= 1)
        topology->support.discovery->pu = 1;
      else
        nbprocs = 1;
      hwloc_alloc_root_sets(topology->levels[0][0]);
      hwloc_setup_pu_level(topology, nbprocs);
    }
  } else if (dstatus->phase == HWLOC_DISC_PHASE_MEMORY) {
      hwloc_look_freebsd_domains(topology);
    #if (defined HAVE_SYSCTL) && (defined HAVE_SYS_SYSCTL_H)
      hwloc_freebsd_node_meminfo_info(topology);
    #endif
      hwloc_obj_add_info(topology->levels[0][0], "Backend", "FreeBSD");
      hwloc_add_uname_info(topology, NULL);
  }
  return 0;
}

void
hwloc_set_freebsd_hooks(struct hwloc_binding_hooks *hooks __hwloc_attribute_unused,
			struct hwloc_topology_support *support __hwloc_attribute_unused)
{
#if defined(HAVE_SYS_CPUSET_H) && defined(HAVE_CPUSET_SETAFFINITY)
  hooks->set_thisproc_cpubind = hwloc_freebsd_set_thisproc_cpubind;
  hooks->get_thisproc_cpubind = hwloc_freebsd_get_thisproc_cpubind;
  hooks->set_thisthread_cpubind = hwloc_freebsd_set_thisthread_cpubind;
  hooks->get_thisthread_cpubind = hwloc_freebsd_get_thisthread_cpubind;
  hooks->set_proc_cpubind = hwloc_freebsd_set_proc_cpubind;
  hooks->get_proc_cpubind = hwloc_freebsd_get_proc_cpubind;
#ifdef hwloc_thread_t
#ifdef HAVE_SYS_DOMAINSET_H
  hooks->set_thisproc_membind = hwloc_freebsd_set_thisproc_membind;
  hooks->get_thisproc_membind = hwloc_freebsd_get_thisproc_membind;
  hooks->set_thisthread_membind = hwloc_freebsd_set_thisthread_membind;
  hooks->get_thisthread_membind = hwloc_freebsd_get_thisthread_membind;
  hooks->set_proc_membind = hwloc_freebsd_set_proc_membind;
  hooks->get_proc_membind = hwloc_freebsd_get_proc_membind;
#endif
#if HAVE_DECL_PTHREAD_SETAFFINITY_NP
  hooks->set_thread_cpubind = hwloc_freebsd_set_thread_cpubind;
#endif
#if HAVE_DECL_PTHREAD_GETAFFINITY_NP
  hooks->get_thread_cpubind = hwloc_freebsd_get_thread_cpubind;
#endif
#endif
#endif
  /* TODO: get_last_cpu_location: find out ki_lastcpu */
}

static struct hwloc_backend *
hwloc_freebsd_component_instantiate(struct hwloc_topology *topology,
				    struct hwloc_disc_component *component,
				    unsigned excluded_phases __hwloc_attribute_unused,
				    const void *_data1 __hwloc_attribute_unused,
				    const void *_data2 __hwloc_attribute_unused,
				    const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;
  backend = hwloc_backend_alloc(topology, component);
  if (!backend)
    return NULL;
  backend->discover = hwloc_look_freebsd;
  return backend;
}

static struct hwloc_disc_component hwloc_freebsd_disc_component = {
  "freebsd",
  HWLOC_DISC_PHASE_CPU|HWLOC_DISC_PHASE_MEMORY,
  HWLOC_DISC_PHASE_GLOBAL,
  hwloc_freebsd_component_instantiate,
  50,
  1,
  NULL
};

const struct hwloc_component hwloc_freebsd_component = {
  HWLOC_COMPONENT_ABI,
  NULL, NULL,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_freebsd_disc_component
};
