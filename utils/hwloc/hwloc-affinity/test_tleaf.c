/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include "utils.h"
#include "tleaf.h"
#include "affinity.h"
#include <assert.h>

hwloc_topology_t knl_topology;
hwloc_topology_t topology;

void test_round_robin(hwloc_topology_t topology)
{
	size_t i, ncore = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	hwloc_obj_t Core;
	hwloc_obj_t it_PU;
	struct cpuaffinity_enum * it;
	it = cpuaffinity_round_robin(topology, HWLOC_OBJ_CORE);
	assert(it != NULL);

	for(i=0; i<cpuaffinity_enum_size(it); i++){
		it_PU = cpuaffinity_enum_next(it);
		Core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i%ncore);
		Core = Core->children[i/ncore];
		assert(it_PU->type == Core->type);
		assert(it_PU->logical_index == Core->logical_index);
	}
	
	cpuaffinity_enum_free(it);
}

void test_scatter(hwloc_topology_t topology)
{
	hwloc_obj_t obj, it_PU;
	ssize_t i, j, r, n, c, val, size, depth, *arities;
	struct cpuaffinity_enum * it;
	
	it = cpuaffinity_scatter(topology, HWLOC_OBJ_PU);
	assert(it != NULL);
	assert(cpuaffinity_enum_size(it) == hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU));
	depth = hwloc_topology_get_depth(topology);
	arities = malloc(depth * sizeof(*arities));
	
	for (i = 0; i < depth-1; i++) {
		obj = hwloc_get_obj_by_depth(topology, i, 0);
		arities[i] = obj->arity;
	}

	for (i = 0; i < cpuaffinity_enum_size(it); i++) {
		it_PU = cpuaffinity_enum_next(it);
		c = i;
		n = cpuaffinity_enum_size(it);
		val = 0;
		for (j = 0; j < depth - 1; j++) {
			r = c % arities[j];
			n = n / arities[j];
			c = c / arities[j];
			val += n * r;
		}
		assert(it_PU->logical_index == val);
	}

	free(arities);
	cpuaffinity_enum_free(it);
}

void test_topology(hwloc_topology_t topology)
{
	test_round_robin(topology);
	test_scatter(topology);
}

int main()
{
	knl_topology = hwloc_affinity_topology_load("knl.xml");
	assert(knl_topology != NULL);
	test_topology(knl_topology);
	hwloc_topology_destroy(knl_topology);

	topology = hwloc_affinity_topology_load(NULL);
	assert(topology != NULL);
	test_topology(topology);
	hwloc_topology_destroy(topology);

	return 0;
}
