/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <hwloc/linux.h>
#include <private/private.h>
#include <private/debug.h>

#ifdef HWLOC_X86_32_ARCH
static inline int have_cpuid(void)
{
  int ret;
  unsigned tmp, tmp2;
  asm(
      "mov $0,%0\n\t"   /* Not supported a priori */

      "pushfl   \n\t"   /* Save flags */

      "pushfl   \n\t"                                           \
      "pop %1   \n\t"   /* Get flags */                         \

#define TRY_TOGGLE                                              \
      "xor $0x00200000,%1\n\t"        /* Try to toggle ID */    \
      "mov %1,%2\n\t"   /* Save expected value */               \
      "push %1  \n\t"                                           \
      "popfl    \n\t"   /* Try to toggle */                     \
      "pushfl   \n\t"                                           \
      "pop %1   \n\t"                                           \
      "cmp %1,%2\n\t"   /* Compare with expected value */       \
      "jnz L1   \n\t"   /* Unexpected, failure */               \

      TRY_TOGGLE        /* Try to set/clear */
      TRY_TOGGLE        /* Try to clear/set */

      "mov $1,%0\n\t"   /* Passed the test! */

      "L1:      \n\t"
      "popfl    \n\t"   /* Restore flags */

      : "=r" (ret), "=&r" (tmp), "=&r" (tmp2));
  return ret;
}
#endif /* HWLOC_X86_32_ARCH */
#ifdef HWLOC_X86_64_ARCH
#define have_cpuid() 1
#endif /* HWLOC_X86_64_ARCH */

static inline void cpuid(unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
  asm("cpuid" : "+a" (*eax), "=b" (*ebx), "+c" (*ecx), "=d" (*edx));
}

struct cacheinfo {
  unsigned type;
  unsigned level;
  unsigned nbthreads_sharing;
  unsigned max_nbcores;
  unsigned nbthreads;

  unsigned linesize;
  unsigned linepart;
  unsigned ways;
  unsigned sets;
  unsigned size;
};

struct procinfo {
  unsigned present;
  unsigned apicid;
  unsigned max_log_proc;
  unsigned socketid;
  unsigned logprocid;
  unsigned threadid;
  unsigned coreid;
  unsigned numcaches;
  struct cacheinfo *cache;
};

/* Fetch information from the processor itself thanks to cpuid and store it in
 * infos for summarize to analyze them globally */
static void look_proc(struct procinfo *infos, unsigned highest_cpuid) {
  unsigned eax, ebx, ecx = 0, edx;
  unsigned cachenum;
  struct cacheinfo *cache;

  infos->present = 1;

  eax = 0x01;
  cpuid(&eax, &ebx, &ecx, &edx);
  infos->apicid = ebx >> 24;
  infos->max_log_proc = (ebx >> 16) & 0xff;
  hwloc_debug("APIC ID 0x%02x max_log_proc %d\n", infos->apicid, infos->max_log_proc);
  infos->socketid = infos->apicid / infos->max_log_proc;
  infos->logprocid = infos->apicid % infos->max_log_proc;
  hwloc_debug("phys %d thread %d\n", infos->socketid, infos->logprocid);

  infos->cache = NULL;

  if (highest_cpuid < 0x04)
    return;

  cachenum = 0;
  infos->numcaches = 0;
  for (cachenum = 0; ; cachenum++) {
    unsigned type;
    eax = 0x04;
    ecx = cachenum;
    cpuid(&eax, &ebx, &ecx, &edx);

    type = eax & 0x1f;

    if (type == 0)
      break;
    if (type == 2)
      /* Instruction cache */
      continue;
    infos->numcaches++;
  }

  cache = infos->cache = malloc(infos->numcaches * sizeof(*infos->cache));

  for (cachenum = 0; ; cachenum++) {
    unsigned linesize, linepart, ways, sets;
    unsigned type;
    eax = 0x04;
    ecx = cachenum;
    cpuid(&eax, &ebx, &ecx, &edx);

    type = eax & 0x1f;

    if (type == 0)
      break;
    if (type == 2)
      /* Instruction cache */
      continue;

    cache->type = type;

    cache->level = (eax >> 5) & 0x7;
    cache->nbthreads_sharing = ((eax >> 14) & 0xfff) + 1;
    cache->max_nbcores = ((eax >> 26) & 0x3f) + 1;

    cache->linesize = linesize = (ebx & 0xfff) + 1;
    cache->linepart = linepart = ((ebx >> 12) & 0x3ff) + 1;
    cache->ways = ways = ((ebx >> 22) & 0x3ff) + 1;
    cache->sets = sets = ecx + 1;
    cache->size = linesize * linepart * ways * sets;

    hwloc_debug("cache %d type %d L%d t%d c%d linesize %d linepart %d ways %d sets %d, size %dKB\n", cachenum, cache->type, cache->level, cache->nbthreads_sharing, cache->max_nbcores, linesize, linepart, ways, sets, cache->size >> 10);
    cache->nbthreads = infos->max_log_proc / cache->max_nbcores;
    hwloc_debug("thus %d threads\n", cache->nbthreads);
    infos->threadid = infos->logprocid % cache->nbthreads;
    infos->coreid = infos->logprocid / cache->nbthreads;
    hwloc_debug("this is thread %d, core %d\n", infos->threadid, infos->coreid);

    cache++;
  }
}

/* Analyse information stored in infos, and build topology levels accordingly */
static void summarize(hwloc_topology_t topology, struct procinfo *infos, unsigned nbprocs)
{
  hwloc_cpuset_t complete_cpuset = hwloc_cpuset_alloc();
  unsigned i, j;

  for (i = 0; i < nbprocs; i++)
    if (infos[i].present)
      hwloc_cpuset_set(complete_cpuset, i);

  /* Look for sockets */
  {
    hwloc_cpuset_t sockets_cpuset = hwloc_cpuset_dup(complete_cpuset);
    hwloc_cpuset_t socket_cpuset;
    hwloc_obj_t socket;

    while ((i = hwloc_cpuset_first(sockets_cpuset)) != (unsigned) -1) {
      unsigned socketid = infos[i].socketid;

      socket_cpuset = hwloc_cpuset_alloc();
      hwloc_cpuset_zero(socket_cpuset);
      for (j = i; j < nbprocs; j++) {
        if (infos[j].socketid == socketid) {
          hwloc_cpuset_set(socket_cpuset, j);
          hwloc_cpuset_clr(sockets_cpuset, j);
        }
      }
      socket = hwloc_alloc_setup_object(HWLOC_OBJ_SOCKET, socketid);
      socket->cpuset = socket_cpuset;
      hwloc_debug_1arg_cpuset("os socket %u has cpuset %s\n",
          socketid, socket_cpuset);
      hwloc_insert_object_by_cpuset(topology, socket);
    }
    hwloc_cpuset_free(sockets_cpuset);
  }

  /* Look for cores */
  {
    hwloc_cpuset_t cores_cpuset = hwloc_cpuset_dup(complete_cpuset);
    hwloc_cpuset_t core_cpuset;
    hwloc_obj_t core;

    while ((i = hwloc_cpuset_first(cores_cpuset)) != (unsigned) -1) {
      unsigned socketid = infos[i].socketid;
      unsigned coreid = infos[i].coreid;

      core_cpuset = hwloc_cpuset_alloc();
      hwloc_cpuset_zero(core_cpuset);
      for (j = i; j < nbprocs; j++) {
        if (infos[j].socketid == socketid && infos[j].coreid == coreid) {
          hwloc_cpuset_set(core_cpuset, j);
          hwloc_cpuset_clr(cores_cpuset, j);
        }
      }
      core = hwloc_alloc_setup_object(HWLOC_OBJ_CORE, coreid);
      core->cpuset = core_cpuset;
      hwloc_debug_1arg_cpuset("os core %u has cpuset %s\n",
          coreid, core_cpuset);
      hwloc_insert_object_by_cpuset(topology, core);
    }
    hwloc_cpuset_free(cores_cpuset);
  }

  /* Look for caches */
  /* First find max level */
  unsigned level = 0, l;
  for (i = 0; i < nbprocs; i++)
    for (j = 0; j < infos[i].numcaches; j++)
      if (infos[i].cache[j].level > level)
        level = infos[i].cache[j].level;

  while (level > 0) {
    /* Look for caches at level level */
    {
      hwloc_cpuset_t caches_cpuset = hwloc_cpuset_dup(complete_cpuset);
      hwloc_cpuset_t cache_cpuset;
      hwloc_obj_t cache;

      while ((i = hwloc_cpuset_first(caches_cpuset)) != (unsigned) -1) {
        unsigned socketid = infos[i].socketid;

        for (l = 0; l < infos[i].numcaches; l++) {
          if (infos[i].cache[l].level == level)
            break;
        }
        if (l == infos[i].numcaches) {
          /* no cache Llevel in i, odd */
          hwloc_cpuset_clr(caches_cpuset, i);
          continue;
        }

        {
          unsigned cacheid = infos[i].apicid / infos[i].cache[l].nbthreads_sharing;

          cache_cpuset = hwloc_cpuset_alloc();
          hwloc_cpuset_zero(cache_cpuset);
          for (j = i; j < nbprocs; j++) {
            unsigned l2;
            for (l2 = 0; l2 < infos[j].numcaches; l2++) {
              if (infos[j].cache[l2].level == level)
                break;
            }
            if (l2 == infos[j].numcaches) {
              /* no cache Llevel in j, odd */
              hwloc_cpuset_clr(caches_cpuset, j);
              continue;
            }
            if (infos[j].socketid == socketid && infos[j].apicid / infos[j].cache[l2].nbthreads_sharing == cacheid) {
              hwloc_cpuset_set(cache_cpuset, j);
              hwloc_cpuset_clr(caches_cpuset, j);
            }
          }
          cache = hwloc_alloc_setup_object(HWLOC_OBJ_CACHE, cacheid);
          cache->attr->cache.depth = level;
          cache->attr->cache.memory_kB = infos[i].cache[l].size >> 10;
          cache->cpuset = cache_cpuset;
          hwloc_debug_2args_cpuset("os L%u cache %u has cpuset %s\n",
              level, cacheid, cache_cpuset);
          hwloc_insert_object_by_cpuset(topology, cache);
        }
      }
      hwloc_cpuset_free(caches_cpuset);
    }
    level--;
  }

  for (i = 0; i < nbprocs; i++)
    free(infos[i].cache);
}

void
hwloc_look_x86(struct hwloc_topology *topology, unsigned nbprocs)
{
  unsigned eax, ebx, ecx = 0, edx;
  hwloc_cpuset_t orig_cpuset;
  unsigned i;
  unsigned highest_cpuid;
  struct procinfo infos[nbprocs];

  if (!have_cpuid())
    return;

  eax = 0x00;
  cpuid(&eax, &ebx, &ecx, &edx);
  highest_cpuid = eax;

  if (highest_cpuid < 0x01)
    /* TODO: AMD version */
    return;

  if (topology->get_thisthread_cpubind && topology->set_thisthread_cpubind) {
    orig_cpuset = topology->get_thisthread_cpubind(topology, HWLOC_CPUBIND_STRICT);
    if (orig_cpuset) {
      hwloc_cpuset_t cpuset = hwloc_cpuset_alloc();
      for (i = 0; i < nbprocs; i++) {
        hwloc_cpuset_cpu(cpuset, i);
        if (topology->set_thisthread_cpubind(topology, cpuset, HWLOC_CPUBIND_STRICT))
          continue;
        look_proc(&infos[i], highest_cpuid);
      }
      hwloc_cpuset_free(cpuset);
      topology->set_thisthread_cpubind(topology, orig_cpuset, 0);
      hwloc_cpuset_free(orig_cpuset);
      summarize(topology, infos, nbprocs);
      return;
    }
  }
  if (topology->get_thisproc_cpubind && topology->set_thisproc_cpubind) {
    orig_cpuset = topology->get_thisproc_cpubind(topology, HWLOC_CPUBIND_STRICT);
    if (orig_cpuset) {
      hwloc_cpuset_t cpuset = hwloc_cpuset_alloc();
      for (i = 0; i < nbprocs; i++) {
        hwloc_cpuset_cpu(cpuset, i);
        if (topology->set_thisproc_cpubind(topology, cpuset, HWLOC_CPUBIND_STRICT))
          continue;
        look_proc(&infos[i], highest_cpuid);
      }
      hwloc_cpuset_free(cpuset);
      topology->set_thisproc_cpubind(topology, orig_cpuset, 0);
      hwloc_cpuset_free(orig_cpuset);
      summarize(topology, infos, nbprocs);
      return;
    }
  }

}
