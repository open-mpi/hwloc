/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "affinity.h"
#include "enum.h"

/** Maximum len of string containing a hwloc_obj logical index **/
#define STR_INT_MAX 5

struct cpuaffinity_enum *cpuaffinity_enum_alloc(hwloc_topology_t topology)
{
	unsigned i, nmax = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);

	struct cpuaffinity_enum *obj = malloc(sizeof *obj);

	if (obj == NULL)
		return NULL;

	obj->topology = topology;
	obj->obj = malloc(nmax * sizeof(*obj->obj));
	if (obj->obj == NULL) {
		free(obj);
		return NULL;
	}

	for (i = 0; i < nmax; i++)
		obj->obj[i] = NULL;
	obj->n = 0;
	obj->nmax = nmax;
	obj->current = 0;

	return obj;
}

void cpuaffinity_enum_free(struct cpuaffinity_enum *obj)
{
	free(obj->obj);
	free(obj);
}

size_t cpuaffinity_enum_size(struct cpuaffinity_enum *obj)
{
	return obj->n;
}

int cpuaffinity_enum_append(struct cpuaffinity_enum *e, hwloc_obj_t obj)
{
	unsigned i;
	if (e == NULL)
		goto out_einval;

	if (e->n == e->nmax)
		goto out_edom;

	if (obj == NULL || (e->obj[0] && obj->type != e->obj[0]->type))
		goto out_einval;

	for (i = 0; i < e->n; i++)
		if (e->obj[i]->logical_index == obj->logical_index)
			goto out_einval;

	e->obj[e->n++] = obj;
	return 0;
out_einval:
	errno = EINVAL;
	return -1;
out_edom:
	errno = EDOM;
	return -1;
}

void cpuaffinity_enum_print(const struct cpuaffinity_enum *e,
			    const char *sep,
			    const int logical,
			    unsigned num)
{
	unsigned i;
	size_t len = e->n * (strlen(sep) + STR_INT_MAX);
	char *c, *enum_str = malloc(len);
	num = num == 0 ? e->n : num;
	num = num > e->n ? e->n : num;

	if (enum_str == NULL) {
		errno = ENOMEM;
		return;
	}

	memset(enum_str, 0, len);
	c = enum_str;

	for (i = 0; i < num-1; i++)
		c += snprintf(c, len + enum_str - c, "%d%s",
			      logical ? e->obj[i]->logical_index : e->obj[i]->
			      os_index, sep);
	c += snprintf(c, len + enum_str - c, "%d",
		      logical ? e->obj[i]->logical_index : e->obj[i]->
		      os_index);

	printf("%s\n", enum_str);
	free(enum_str);
}

hwloc_obj_t cpuaffinity_enum_next(struct cpuaffinity_enum *e)
{
	hwloc_obj_t obj = e->obj[e->current];
	e->current = (e->current + 1) % e->n;
        return obj;
}

hwloc_obj_t cpuaffinity_enum_get(struct cpuaffinity_enum * e,
				 const size_t i)
{
	if(e == NULL)
		return NULL;
	return e->obj[i % e->n];
}
