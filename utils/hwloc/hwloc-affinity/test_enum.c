/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include "utils.h"
#include "affinity.h"
#include <assert.h>

hwloc_topology_t knl_topology;
hwloc_topology_t topology;

void test_enum(hwloc_topology_t topology)
{
	unsigned i;
	hwloc_obj_t obj, obj_e;
	struct cpuaffinity_enum *it, *it_e;

	it_e = cpuaffinity_enum_alloc(topology);
	it = cpuaffinity_round_robin(topology, HWLOC_OBJ_PU);
	assert(it != NULL);
	assert(cpuaffinity_enum_size(it) == hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU));	
	
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		assert(cpuaffinity_enum_append(it_e, obj) == 0);
	}
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		obj_e = cpuaffinity_enum_next(it_e);
		assert(obj == obj_e);
	}

	cpuaffinity_enum_free(it);
	cpuaffinity_enum_free(it_e);
}

int main()
{
	knl_topology = hwloc_affinity_topology_load("knl.xml");
	assert(knl_topology != NULL);
	test_enum(knl_topology);
	hwloc_topology_destroy(knl_topology);

	topology = hwloc_affinity_topology_load(NULL);
	assert(topology != NULL);
	test_enum(topology);
	hwloc_topology_destroy(topology);
	return 0;
}
