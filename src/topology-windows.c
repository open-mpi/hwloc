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

#include <topology.h>
#include <topology/helper.h>
#include <topology/debug.h>

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
look_procInfo(struct topo_topology *topology, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo, int n, LOGICAL_PROCESSOR_RELATIONSHIP relationship, topo_obj_type_t type, int cacheLevel)
{
  int i, j;
  struct topo_obj *obj;

  /* Ignore non-data caches and other levels */
#define TEST \
    procInfo[i].Relationship == relationship \
	&& (relationship != RelationCache || \
	  ((procInfo[i].Cache.Type == CacheUnified \
		|| procInfo[i].Cache.Type == CacheData) \
	   && procInfo[i].Cache.Level == cacheLevel) \
	   )

  topo_debug("relation %d type %d L%d\n", relationship, type, cacheLevel);

  j = 0;
  for (i = 0; i < n; i++) {
    if (TEST) {
      obj = malloc(sizeof(struct topo_obj));
      assert(obj);
      topo_setup_object(obj, type, j);
      topo_debug("%d #%d mask %lx\n", relationship, j, procInfo[i].ProcessorMask);
      topo_cpuset_from_ulong(&obj->cpuset, procInfo[i].ProcessorMask);
      switch (type) {
	case TOPO_OBJ_NODE:
	  obj->memory_kB = 0; /* TODO */
	  obj->huge_page_free = 0; /* TODO */
	  obj->physical_index = procInfo[i].NumaNode.NodeNumber; /* override what topo_setup_object did */
	  break;
	case TOPO_OBJ_CACHE:
	  obj->memory_kB = procInfo[i].Cache.Size >> 10;
	  obj->cache_depth = cacheLevel;
	  break;
	default:
	  break;
      }
      j++;
      topo_add_object(topology, obj);
    }
  }

#undef TEST
}

void
topo_look_windows(struct topo_topology *topology)
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

      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationNumaNode, TOPO_OBJ_NODE, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationProcessorPackage, TOPO_OBJ_SOCKET, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_OBJ_CACHE, 3);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_OBJ_CACHE, 2);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationProcessorCore, TOPO_OBJ_CORE, 0);
      look_procInfo(topology, procInfo, length / sizeof(*procInfo), RelationCache, TOPO_OBJ_CACHE, 1);

      free(procInfo);
    }
  }
}
