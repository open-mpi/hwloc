/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

#include <windows.h>

#ifndef HAVE_KAFFINITY
typedef ULONG_PTR KAFFINITY, *PKAFFINITY;
#endif

#ifndef HAVE_PROCESSOR_CACHE_TYPE
typedef enum _PROCESSOR_CACHE_TYPE {
  CacheUnified,
  CacheInstruction,
  CacheData,
  CacheTrace
} PROCESSOR_CACHE_TYPE;
#endif

#ifndef CACHE_FULLY_ASSOCIATIVE
#define CACHE_FULLY_ASSOCIATIVE 0xFF
#endif

#ifndef HAVE_CACHE_DESCRIPTOR
typedef struct _CACHE_DESCRIPTOR {
  BYTE Level;
  BYTE Associativity;
  WORD LineSize;
  DWORD Size; /* in bytes */
  PROCESSOR_CACHE_TYPE Type;
} CACHE_DESCRIPTOR, *PCACHE_DESCRIPTOR;
#endif

#ifndef HAVE_LOGICAL_PROCESSOR_RELATIONSHIP
typedef enum _LOGICAL_PROCESSOR_RELATIONSHIP {
  RelationProcessorCore,
  RelationNumaNode,
  RelationCache,
  RelationProcessorPackage,
  RelationGroup,
  RelationAll = 0xffff,
} LOGICAL_PROCESSOR_RELATIONSHIP;
#endif

#ifndef HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION {
  ULONG_PTR ProcessorMask;
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  _ANONYMOUS_UNION
  union {
    struct {
      BYTE flags;
    } ProcessorCore;
    struct {
      DWORD NodeNumber;
    } NumaNode;
    CACHE_DESCRIPTOR Cache;
    ULONGLONG Reserved[2];
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION;
#endif

/* Extended interface, for group support */

#ifndef HAVE_GROUP_AFFINITY
typedef struct _GROUP_AFFINITY {
  KAFFINITY Mask;
  WORD Group;
  WORD Reserved[3];
} GROUP_AFFINITY, *PGROUP_AFFINITY;
#endif

#ifndef HAVE_PROCESSOR_RELATIONSHIP
typedef struct _PROCESSOR_RELATIONSHIP {
  BYTE Flags;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} PROCESSOR_RELATIONSHIP, *PPROCESSOR_RELATIONSHIP;
#endif

#ifndef HAVE_NUMA_NODE_RELATIONSHIP
typedef struct _NUMA_NODE_RELATIONSHIP {
  DWORD NodeNumber;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} NUMA_NODE_RELATIONSHIP, *PNUMA_NODE_RELATIONSHIP;
#endif

#ifndef HAVE_CACHE_RELATIONSHIP
typedef struct _CACHE_RELATIONSHIP {
  BYTE Level;
  BYTE Associativity;
  WORD LineSize;
  DWORD CacheSize;
  PROCESSOR_CACHE_TYPE Type;
  ULONGLONG Reserved[2];
  GROUP_AFFINITY GroupMask;
} CACHE_RELATIONSHIP, *PCACHE_RELATIONSHIP;
#endif

#ifndef HAVE_PROCESSOR_GROUP_INFO
typedef struct _PROCESSOR_GROUP_INFO {
  BYTE MaximumProcessorCount;
  BYTE ActiveProcessorCount;
  KAFFINITY ActiveProcessorMask;
  ULONGLONG Reserved[4];
} PROCESSOR_GROUP_INFO, *PPROCESSOR_GROUP_INFO;
#endif

#ifndef HAVE_GROUP_RELATIONSHIP
typedef struct _GROUP_RELATIONSHIP {
  WORD MaximumGroupCount;
  WORD ActiveGroupCount;
  ULONGLONG Reserved[2];
  PROCESSOR_GROUP_INFO GroupInfo[ANYSIZE_ARRAY];
} GROUP_RELATIONSHIP, *PGROUP_RELATIONSHIP;
#endif

#ifndef HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX
typedef struct _SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX {
  LOGICAL_PROCESSOR_RELATIONSHIP Relationship;
  DWORD Size;
  _ANONYMOUS_UNION
  union {
    PROCESSOR_RELATIONSHIP Processor;
    NUMA_NODE_RELATIONSHIP NumaNode;
    CACHE_RELATIONSHIP Cache;
    GROUP_RELATIONSHIP Group;
    /* Odd: no member to tell the cpu mask of the package... */
  } DUMMYUNIONNAME;
} SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, *PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX;
#endif

static void
look_procInfo(struct topo_topology *topology, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo, int n, LOGICAL_PROCESSOR_RELATIONSHIP relationship, topo_level_type_t type, int cacheLevel)
{
  int i, j;
  int numitems;
  struct topo_level *level;

  /* Ignore non-data caches and other levels */
#define TEST \
    procInfo[i].Relationship == relationship \
	&& (relationship != RelationCache || \
	  ((procInfo[i].Cache.Type == CacheUnified \
		|| procInfo[i].Cache.Type == CacheData) \
	   && procInfo[i].Cache.Level == cacheLevel) \
	   )

  numitems = 0;
  for (i = 0; i < n; i++)
    if (TEST)
      numitems++;

  topo_debug("relation %d type %d num %d L%d\n", relationship, type, numitems, cacheLevel);
  if (!numitems)
    return;

  level = malloc((numitems+1) * sizeof(*level));
  j = 0;
  for (i = 0; i < n; i++) {
    if (TEST) {
      topo_setup_level(&level[j], type, j);
      topo_debug("%d #%d mask %lx\n", relationship, j, procInfo[i].ProcessorMask);
      topo_cpuset_from_ulong(&level[j].cpuset, procInfo[i].ProcessorMask);
      switch (type) {
	case TOPO_LEVEL_NODE:
	  level[j].memory_kB[TOPO_LEVEL_MEMORY_NODE] = 0; /* TODO */
	  level[j].huge_page_free = 0; /* TODO */
	  level[j].physical_index = procInfo[i].NumaNode.NodeNumber; /* override what topo_level_setup did */
	  break;
	case TOPO_LEVEL_L1:
	  level[j].memory_kB[TOPO_LEVEL_MEMORY_L1] = procInfo[i].Cache.Size >> 10;
	  break;
	case TOPO_LEVEL_L2:
	  level[j].memory_kB[TOPO_LEVEL_MEMORY_L2] = procInfo[i].Cache.Size >> 10;
	  break;
	case TOPO_LEVEL_L3:
	  level[j].memory_kB[TOPO_LEVEL_MEMORY_L3] = procInfo[i].Cache.Size >> 10;
	  break;
	default:
	  break;
      }
      j++;
    }
  }

  topo_cpuset_zero(&level[j].cpuset);

  topology->level_nbitems[topology->nb_levels] = numitems;
  topology->levels[topology->nb_levels++] = level;

#undef TEST
}

void
look_windows(struct topo_topology *topology)
{
  BOOL WINAPI (*GetLogicalProcessorInformationProc)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength);
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo;
  DWORD length;

  HMODULE kernel32;

  kernel32 = LoadLibrary("kernel32.dll");
  if (kernel32) {
    /* TODO: GetLogicalProcessorInformationEx to go beyond 64 CPUs.  */
    GetLogicalProcessorInformationProc = GetProcAddress(kernel32, "GetLogicalProcessorInformation");

    if (GetLogicalProcessorInformationProc) {
      length = 0;
      procInfo = NULL;

      while (1) {
	if (GetLogicalProcessorInformationProc(procInfo, &length))
	  break;
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	  return;
	free(procInfo);
	procInfo = malloc(length);
      }

      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationNumaNode, TOPO_LEVEL_NODE, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationProcessorPackage, TOPO_LEVEL_DIE, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_LEVEL_L3, 3);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_LEVEL_L2, 2);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationProcessorCore, TOPO_LEVEL_CORE, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_LEVEL_L1, 1);

      free(procInfo);
    }
  }

  /* we have a contigous range of online cpus */
  topo_cpuset_set_range(&topology->online_cpuset, 0, topology->nb_processors-1);
}
