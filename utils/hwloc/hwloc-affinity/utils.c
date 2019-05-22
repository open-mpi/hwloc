/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include "utils.h"
#include <errno.h>

hwloc_topology_t hwloc_affinity_topology_load(const char *file)
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

	/* hwloc_topology_set_all_types_filter(topology, */
	/* 				    HWLOC_TYPE_FILTER_KEEP_STRUCTURE); */
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

hwloc_obj_t hwloc_obj_from_string(hwloc_topology_t topology, const char *str)
{
	hwloc_obj_t obj;
	hwloc_obj_type_t type;
	int id;
	char *obj_str, *save_ptr, *type_str, *id_str;

	obj_str = strdup(str);
	save_ptr = obj_str;
	if (obj_str == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	type_str = strtok(obj_str, ":");
	if (type_str == NULL)
		goto out_einval;

	if (hwloc_type_sscanf(type_str, &type, NULL, 0) < 0)
		goto out_einval;

	id_str = strtok(NULL, ":");

	if (id_str == NULL)
		goto out_einval;

	id = atoi(id_str);
	if (id < 0 || id >= hwloc_get_nbobjs_by_type(topology, type))
		goto out_einval;

	obj = hwloc_get_obj_by_type(topology, type, atoi(id_str));
	if (obj == NULL)
		goto out_einval;

	free(save_ptr);
	return obj;

out_einval:
	free(save_ptr);
	errno = EINVAL;
	return NULL;
}

int hwloc_topology_restrict_obj(hwloc_topology_t topology, hwloc_obj_t obj)
{
	if (hwloc_bitmap_weight(obj->cpuset) > 0) {
		return hwloc_topology_restrict(topology, obj->cpuset,
					       HWLOC_RESTRICT_FLAG_REMOVE_CPULESS);
	} else if (hwloc_bitmap_weight(obj->nodeset) > 0) {
		return hwloc_topology_restrict(topology, obj->nodeset,
					       HWLOC_RESTRICT_FLAG_BYNODESET |
					       HWLOC_RESTRICT_FLAG_REMOVE_MEMLESS);
	}
	errno = EINVAL;
	return -1;
}
