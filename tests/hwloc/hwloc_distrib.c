/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
 * See COPYING in top-level directory.
****************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include "private/autogen/config.h"
#include "hwloc.h"
#include "hwloc/helper.h"

static hwloc_topology_t hwloc_test_topology_load(const char *file)
{
	hwloc_topology_t topology;

	if (hwloc_topology_init(&topology)) {
		perror("hwloc_topology_init");
		goto error;
	}

	if (file != NULL && hwloc_topology_set_xml(topology, file) != 0) {
		perror("hwloc_topology_set_xml");
		goto error;
	}

	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PU,
				       HWLOC_TYPE_FILTER_KEEP_ALL);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_NUMANODE,
				       HWLOC_TYPE_FILTER_KEEP_ALL);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PCI_DEVICE,
				       HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_OS_DEVICE,
				       HWLOC_TYPE_FILTER_KEEP_NONE);

	if (hwloc_topology_load(topology) != 0) {
		perror("hwloc_topology_load");
		goto error_with_topology;
	}

	return topology;

error_with_topology:
	hwloc_topology_destroy(topology);
error:
	return NULL;
}

static int is_tleaf(hwloc_topology_t topology)
{
	hwloc_obj_t next, obj = hwloc_get_obj_by_depth(topology, 0, 0);
	while(obj){
		next = obj->next_cousin;
		while(next){
			if(next->arity != obj->arity)
				return 0;
			next = next->next_cousin;
		}
		obj = obj->first_child;
	}
	return 1;
}

static void test_round_robin_PU(hwloc_topology_t topology)
{
	int i,nPU = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	struct hwloc_distrib_iterator *it;
	hwloc_obj_t PU, item;
	
	it = hwloc_distrib_iterator_round_robin(topology, HWLOC_OBJ_PU, 0);
	assert(it != NULL);
        
	for(i=0; i<nPU; i++){
	        hwloc_distrib_iterator_next(topology, it, &item);
	        PU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
		assert(PU == item);
	}

	hwloc_distrib_destroy_iterator(it);
}

static void test_reversed_round_robin_PU(hwloc_topology_t topology)
{
	int i,nPU = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	struct hwloc_distrib_iterator *it;
	hwloc_obj_t PU, item;
	
	it = hwloc_distrib_iterator_round_robin(topology,
						HWLOC_OBJ_PU,
						HWLOC_DISTRIB_FLAG_REVERSE);
	assert(it != NULL);
        
	for(i=0; i<nPU; i++){
	        hwloc_distrib_iterator_next(topology, it, &item);
	        PU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, nPU-i-1);
		assert(PU == item);
	}

	hwloc_distrib_destroy_iterator(it);
}

static void test_round_robin_Core_PU(hwloc_topology_t topology)
{
	// This test can only work if all the Cores have the same amount of child PU.
	if ( !is_tleaf(topology) )
		return;
	if ( hwloc_get_type_depth(topology, HWLOC_OBJ_CORE) < 0 )
		return;
	if ( hwloc_get_type_depth(topology, HWLOC_OBJ_PU) < 0 )
		return;
	
	int i;
	int nPU = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	int ncore = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	
	size_t n_levels = 2;
	hwloc_obj_type_t levels[2] = {HWLOC_OBJ_PU, HWLOC_OBJ_CORE};

	hwloc_obj_t root = hwloc_get_obj_by_depth(topology, 0, 0);
	struct hwloc_distrib_iterator *it;
	hwloc_obj_t core, PU, item;
	
	it = hwloc_distrib_build_iterator(topology, &root, 1, levels, n_levels, 0);
	assert(it != NULL);
	
	for(i=0; i<nPU; i++){
	        hwloc_distrib_iterator_next(topology, it, &item);		
	        core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i%ncore);
		PU = hwloc_get_obj_inside_cpuset_by_type(topology,
							 core->cpuset,
							 HWLOC_OBJ_PU, i/ncore);
		assert(PU == item);
	}
	hwloc_distrib_destroy_iterator(it);
}

static void test_scatter(hwloc_topology_t topology)
{
	// This test works only if topology is a tleaf.
	if ( !is_tleaf(topology) )
		return;

	ssize_t i=0, j, r, n_levels=0, n=0, c, val, nleaves=1;
	hwloc_obj_t item, obj, root = hwloc_get_obj_by_depth(topology, 0, 0);
	struct hwloc_distrib_iterator *it;

	it = hwloc_distrib_iterator_scatter(topology, HWLOC_OBJ_PU, 0);
	assert(it != NULL);

	obj = root;
	while(obj){
		if ((obj->cpuset != NULL && !hwloc_bitmap_iszero(obj->cpuset)) &&
		    hwloc_get_type_depth(topology, obj->type) >= 0)
			n++;
		obj = obj->first_child;
	}
	assert(n != 0);

	ssize_t arities[n];

	item = obj = root;
 next_level:
	do {
		obj = obj->first_child;
	} while( obj != NULL && (obj->cpuset == NULL ||
				 hwloc_bitmap_iszero(obj->cpuset) ||
				 hwloc_get_type_depth(topology, obj->type) < 0));
	if(obj != NULL){
		arities[n-1-i] = hwloc_get_nbobjs_inside_cpuset_by_type(topology,
									item->cpuset,
									obj->type);
		nleaves *= arities[n-1-i];
		i++;		
		item = obj;
		goto next_level;
	}

	n_levels=n;
	for (i = 0; hwloc_distrib_iterator_next(topology, it, &item); i++) {
		c = i;
		n = nleaves;
		val = 0;
		for (j = (n_levels-1); j > 0; j--) {
			r = c % arities[j];
			n = n / arities[j];
			c = c / arities[j];
			val += n * r;
		}
		assert(item->logical_index == val);
	}

	hwloc_distrib_destroy_iterator(it);
}

/***************************************************************************/
/*                                  Run Tests                              */
/***************************************************************************/

int main(void)
{
	hwloc_topology_t topology;
	DIR* xml_dir = opendir(XMLTESTDIR);
	if(xml_dir == NULL){
		perror("opendir");
		return 1;
	}

	char fname[512];
	struct dirent *dirent;
	for(dirent = readdir(xml_dir);
	    dirent != NULL;
	    dirent = readdir(xml_dir)){
		// Not supported by solaris and not critical.
		/* if(dirent->d_type != DT_REG) */
		/* 	continue; */
		if(strcmp(dirent->d_name + strlen(dirent->d_name) - 4,
			  ".xml"))
			continue;
		memset(fname, 0, sizeof(fname));
		snprintf(fname,
			 sizeof(fname),
			 "%s/%s",
			 XMLTESTDIR,
			 dirent->d_name);
		topology = hwloc_test_topology_load(fname);
		if(topology == NULL)
			continue;
		test_round_robin_PU(topology);
		test_reversed_round_robin_PU(topology);
		test_round_robin_Core_PU(topology);
	        test_scatter(topology);
		hwloc_topology_destroy(topology);
	}
	
	closedir(xml_dir);
	return 0;
}

