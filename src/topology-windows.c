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

/* To try to get all declarations duplicated below.  */
#define _WIN32_WINNT 0x0601

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


void
topo_look_windows(struct topo_topology *topology)
{
  BOOL WINAPI (*GetLogicalProcessorInformationProc)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength);
  BOOL WINAPI (*GetLogicalProcessorInformationExProc)(LOGICAL_PROCESSOR_RELATIONSHIP relationship, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer, PDWORD ReturnLength);
  DWORD length;

  HMODULE kernel32;

  kernel32 = LoadLibrary("kernel32.dll");
  if (kernel32) {
    GetLogicalProcessorInformationProc = GetProcAddress(kernel32, "GetLogicalProcessorInformation");

    if (GetLogicalProcessorInformationProc) {
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION procInfo;
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

      unsigned id;
      int i;
      struct topo_obj *obj;
      enum topo_obj_type_e type;

      for (i = 0; i < length / sizeof(*procInfo); i++) {

        /* Ignore non-data caches */
	if (procInfo[i].Relationship == RelationCache
		&& procInfo[i].Cache.Type != CacheUnified
		&& procInfo[i].Cache.Type != CacheData)
	  continue;

	id = -1;
	switch (procInfo[i].Relationship) {
	  case RelationNumaNode:
	    type = TOPO_OBJ_NODE;
	    id = procInfo[i].NumaNode.NodeNumber;
	    break;
	  case RelationProcessorPackage:
	    type = TOPO_OBJ_SOCKET;
	    break;
	  case RelationCache:
	    type = TOPO_OBJ_CACHE;
	    break;
	  case RelationProcessorCore:
	    type = TOPO_OBJ_CORE;
	    break;
	  case RelationGroup:
	  default:
	    type = TOPO_OBJ_FAKE;
	    break;
	}

	obj = topo_alloc_setup_object(type, id);
	topo_debug("%s#%d mask %lx\n", topo_object_type_string(type), id, procInfo[i].ProcessorMask);
	topo_cpuset_from_ulong(&obj->cpuset, procInfo[i].ProcessorMask);

	switch (type) {
	  case TOPO_OBJ_NODE:
	    obj->memory_kB = 0; /* TODO */
	    obj->huge_page_free = 0; /* TODO */
	    break;
	  case TOPO_OBJ_CACHE:
	    obj->memory_kB = procInfo[i].Cache.Size >> 10;
	    obj->cache_depth = procInfo[i].Cache.Level;
	    break;
	  case TOPO_OBJ_FAKE:
	    obj->fake_depth = procInfo[i].Relationship == RelationGroup;
	    break;
	  default:
	    break;
	}
	topo_add_object(topology, obj);
      }

      free(procInfo);
    }

    GetLogicalProcessorInformationExProc = GetProcAddress(kernel32, "GetLogicalProcessorInformationEx");

    /* Disabled for now as it wasn't tested at all.  */
    if (0 && GetLogicalProcessorInformationExProc) {
      PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procInfoTotal, procInfo;

      fprintf(stderr,"Note: GetLogicalProcessorInformationEx was never tested yet!\n");

      length = 0;
      procInfoTotal = NULL;

      while (1) {
	if (GetLogicalProcessorInformationExProc(RelationAll, procInfoTotal, &length))
	  break;
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	  return;
	free(procInfoTotal);
	procInfo = malloc(length);
      }

      unsigned next_group_id = 0, id;
      struct topo_obj *obj;
      enum topo_obj_type_e type;
      KAFFINITY mask;
      WORD group;

      for (procInfo = procInfoTotal;
	   (void*) procInfo < (void*) ((unsigned long) procInfoTotal + length);
	   procInfo = (void*) ((unsigned long) procInfo + procInfo->Size)) {

        /* Ignore non-data caches */
	if (procInfo->Relationship == RelationCache &&
		  (procInfo->Cache.Type == CacheUnified
		|| procInfo->Cache.Type == CacheData))
	  continue;

	id = -1;
	switch (procInfo->Relationship) {
	  case RelationNumaNode:
	    type = TOPO_OBJ_NODE;
	    mask = procInfo->NumaNode.GroupMask.Mask;
	    group = procInfo->NumaNode.GroupMask.Group;
	    id = procInfo->NumaNode.NodeNumber;
	    break;
	  case RelationProcessorPackage:
#if 0
	    /* MSDN says `There is no additional information available.', how
	     * can we know which processors are there?!  Some sample code on
	     * the Internet use the Processor field.  */
	    type = TOPO_OBJ_SOCKET;
	    mask = procInfo->Processor.GroupMask.Mask;
	    group = procInfo->Processor.GroupMask.Group;
	    id = type_id[type]++;
	    break;
#else
	    continue;
#endif
	  case RelationCache:
	    type = TOPO_OBJ_CACHE;
	    mask = procInfo->Cache.GroupMask.Mask;
	    group = procInfo->Cache.GroupMask.Group;
	    break;
	  case RelationProcessorCore:
	    type = TOPO_OBJ_CORE;
	    mask = procInfo->Processor.GroupMask.Mask;
	    group = procInfo->Processor.GroupMask.Group;
	    break;
	  case RelationGroup:
	    type = TOPO_OBJ_FAKE;
	    /* 0 is very odd, does Windows report only one RelationGroup item?  */
	    /* Some sample code on the Internet assume that
	     * Group.ActiveGroupCount gives the size of the GroupInfo array. */
	    mask = procInfo->Group.GroupInfo[0].ActiveProcessorMask;
	    group = id = next_group_id++;
	    break;
	  default:
	    /* Don't know how to get the mask.  */
	    continue;
	}

	obj = topo_alloc_setup_object(type, id);
	topo_debug("%s#%d mask %d:%lx\n", topo_object_type_string(type), id, group, mask);
	topo_cpuset_from_ith_ulong(&obj->cpuset, group, mask);

	switch (type) {
	  case TOPO_OBJ_NODE:
	    obj->memory_kB = 0; /* TODO */
	    obj->huge_page_free = 0; /* TODO */
	    break;
	  case TOPO_OBJ_CACHE:
	    obj->memory_kB = procInfo->Cache.CacheSize >> 10;
	    obj->cache_depth = procInfo->Cache.Level;
	    break;
	  default:
	    break;
	}
	topo_add_object(topology, obj);
      }
      free(procInfoTotal);
    }
  }

  /* add PROC objects */
  topo_setup_proc_level(topology, NULL);
}
