/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include <stdlib.h>
#include "affinity.h"
#include "utils.h"
#include "tleaf.h"

struct hwloc_tleaf_iterator {
	hwloc_topology_t topology;
	int n;			// Actual number of levels in hwloc_tleaf_iterator
	unsigned *depth;	// Topology depth of levels.
	unsigned *asc_order;	// Index for reordering levels in ascending order.
	unsigned *arity;	// Number of elements at level below parent.
	unsigned *it;		// Current iterator index of level.w
	unsigned **index;	// For each level indexing of elements.
};

static inline void shuffle(unsigned *x, const size_t n)
{
	size_t i;
	unsigned swap, rnd;

	for (i = 0; i < n; i++) {
		swap = x[i];
		rnd = rand() % (n - i);
		x[i] = x[i + rnd];
		x[i + rnd] = swap;
	}
}

static inline unsigned *build_index(const size_t n)
{
	size_t i;
	unsigned *ind = malloc(n * sizeof(*ind));
	if (ind == NULL)
		return NULL;
	for (i = 0; i < n; i++)
		ind[i] = i;
	return ind;
}

static inline int comp_unsigned(const void *a_ptr, const void *b_ptr)
{
	const unsigned a = *(unsigned *)a_ptr;
	const unsigned b = *(unsigned *)b_ptr;
	if (a < b)
		return 1;
	if (a > b)
		return -1;
	return 0;
}

static inline int is_child(const hwloc_obj_t parent, hwloc_obj_t child)
{
	if (child->depth <= parent->depth)
		return 0;
	while (child != NULL && child->depth > parent->depth)
		child = child->parent;
	if (child == NULL)
		return 0;
	if (child->logical_index != parent->logical_index)
		return 0;
	return 1;
}

static inline unsigned num_children(const struct hwloc_tleaf_iterator *t,
				    const unsigned parent_depth,
				    const unsigned child_depth)
{
	unsigned arity = 0, n = 0;
	hwloc_obj_t c, p = hwloc_get_obj_by_depth(t->topology, parent_depth, 0);

	while(p){
		c = hwloc_get_obj_by_depth(t->topology, child_depth, 0);
		n = 0;
		
		while (c != NULL && is_child(p, c)) {
			c = c->next_cousin;
			n++;
		}
		arity = n > arity ? n : arity;
		p = p->next_cousin;
	}
	return arity;
}

static inline hwloc_obj_t get_child(const hwloc_obj_t parent,
				    const int child_depth,
				    unsigned child_index)
{
	hwloc_obj_t child = parent;
	while (child && child->depth < child_depth)
		child = child->first_child;
	while (child && child_index--)
		child = child->next_cousin;
	return child;
}

#define chk_ptr(ptr, goto_flag) if(ptr == NULL){errno = ENOMEM; goto goto_flag;}

struct hwloc_tleaf_iterator *
hwloc_tleaf_iterator_alloc(hwloc_topology_t topology,
			   const int n_levels,
			   const hwloc_obj_type_t *levels)
{
	int i, n, depth, cd, pd;
	struct hwloc_tleaf_iterator *t;

	t = malloc(sizeof(*t));
	if (t == NULL)
		return NULL;

	t->depth = malloc(n_levels * sizeof(*t->depth));
	chk_ptr(t->depth, error);

	t->asc_order = malloc(n_levels * sizeof(*t->asc_order));
	chk_ptr(t->asc_order, error_with_depth);

	t->arity = malloc(n_levels * sizeof(*t->arity));
	chk_ptr(t->arity, error_with_order);

	t->it = malloc(n_levels * sizeof(*t->it));
	chk_ptr(t->it, error_with_size);

	t->index = malloc(n_levels * sizeof(*t->index));
	chk_ptr(t->index, error_with_it);

        t->topology = topology;

	t->n = n_levels;

	// Assign depths and set iteration step
	for (i = 0; i < n_levels; i++) {
		depth = hwloc_get_type_depth(topology, levels[i]);
		if (depth < 0) {
			errno = EINVAL;
			goto error_with_index;
		}
		t->depth[i] = depth;
		t->it[i] = 0;
	}

	// order depths in asc_order
	memcpy(t->asc_order, t->depth, n_levels * sizeof(*t->asc_order));
	qsort(t->asc_order, n_levels, sizeof(*t->asc_order), comp_unsigned);

	// replace asc_order with index of matching element in depth.
	for (n = 0; n < t->n; n++)
		for (i = 0; i < t->n; i++)
			if (t->depth[i] == t->asc_order[n]) {
				t->asc_order[n] = i;
				break;
			}
	// Compute levels arity.
	for (i = 1; i < n_levels; i++) {
		pd = t->depth[t->asc_order[i]];
		cd = t->depth[t->asc_order[i - 1]];
		t->arity[t->asc_order[i - 1]] = num_children(t, pd, cd);
	}
	t->arity[t->asc_order[t->n - 1]] =
	    hwloc_get_nbobjs_by_depth(t->topology,
				      t->depth[t->asc_order[t->n - 1]]);

	// Allocate iteration index for levels
	for (i = 0; i < n_levels; i++) {
		t->index[i] = build_index(t->arity[i]);
		if (t->index[i] == NULL) {
			while (i--)
				free(t->index[i]);
			errno = ENOMEM;
			goto error_with_index;
		}
	}

	return t;

error_with_index:
	free(t->index);
error_with_it:
	free(t->it);
error_with_size:
	free(t->arity);
error_with_order:
	free(t->asc_order);
error_with_depth:
	free(t->depth);
error:
	free(t);
	return NULL;
}

void hwloc_tleaf_iterator_free(struct hwloc_tleaf_iterator *t)
{
	int i;
	if (t == NULL)
		return;

	free(t->depth);
	free(t->asc_order);
	free(t->arity);
	free(t->it);
	for (i = 0; i < t->n; i++)
		free(t->index[i]);
	free(t->index);
	free(t);
}

size_t hwloc_tleaf_iterator_size(const struct hwloc_tleaf_iterator *t)
{
        int i;
	size_t s = 1;
	if (t == NULL || t->n == 0)
		return -1;
	for (i = 0; i < t->n; i++)
		s *= t->arity[i];
	return s;
}

void hwloc_tleaf_iterator_shuffle(struct hwloc_tleaf_iterator *t)
{
	int i;
	for (i = 0; i < t->n; i++)
		shuffle(t->index[i], t->arity[i]);
}

hwloc_obj_t hwloc_tleaf_iterator_current(const struct hwloc_tleaf_iterator *t)
{
	if (t == NULL)
		return NULL;

	hwloc_obj_t p;
	int i, j;

	i = t->asc_order[t->n - 1];
	p = hwloc_get_obj_by_depth(t->topology, t->depth[i],
				   t->index[i][t->it[i]]);

	if (p == NULL)
		return NULL;

	for (i = t->n - 2; i >= 0; i--) {
		j = t->asc_order[i];
		p = get_child(p, t->depth[j], t->index[j][t->it[j]]);
	}

	return p;
}

hwloc_obj_t hwloc_tleaf_iterator_next(struct hwloc_tleaf_iterator * t)
{
	int i;
	hwloc_obj_t current;

	if (t == NULL || t->n < 2)
		return NULL;

	current = hwloc_tleaf_iterator_current(t);
	if (current == NULL)
		return NULL;

	i = 0;
	while (i < t->n && t->it[i] == (t->arity[i] - 1))
		t->it[i++] = 0;

	if (i < t->n)
		t->it[i]++;

	return current;
}

struct cpuaffinity_enum *hwloc_tleaf_iterator_enumeration(struct
						       hwloc_tleaf_iterator *it)
{
	int i, n = 1;
	hwloc_obj_t obj;
	
	if (it == NULL)
		return NULL;

	for (i = 0; i < it->n; i++)
		n *= it->arity[i];

	struct cpuaffinity_enum *e = cpuaffinity_enum_alloc(it->topology);

	if (e == NULL)
		return NULL;

	for (i = 0; i < n; i++){
		obj = hwloc_tleaf_iterator_next(it);
		if(obj)
			cpuaffinity_enum_append(e, obj);
	}

	return e;
}

struct cpuaffinity_enum *
cpuaffinity_tleaf(hwloc_topology_t topology,
		  const size_t n_levels,
		  const hwloc_obj_type_t *levels,
		  const int shuffle)
{
	int new_topo = topology == NULL ? 1 : 0;
	struct hwloc_tleaf_iterator * it;
	struct cpuaffinity_enum * e;
	if(n_levels <= 1){
		errno = EINVAL;
		return NULL;
	}
		
	if(new_topo){
		topology = hwloc_affinity_topology_load(NULL);
		if(topology == NULL)
			return NULL;
	}
	it = hwloc_tleaf_iterator_alloc(topology, n_levels, levels);
	if(it == NULL)
		return NULL;

	if(shuffle)
		hwloc_tleaf_iterator_shuffle(it);

	
	e = hwloc_tleaf_iterator_enumeration(it);

	// Cleanup
	hwloc_tleaf_iterator_free(it);
	if(new_topo)
		hwloc_topology_destroy(topology);
	
	return e;
}

static struct cpuaffinity_enum *
cpuaffinity_default_policy(hwloc_topology_t topology,
			   const hwloc_obj_type_t level, int scatter)
{
	hwloc_obj_t obj;
	int i, ldepth;
	hwloc_obj_type_t *types;
        struct hwloc_tleaf_iterator *it = NULL;
	struct cpuaffinity_enum *e = NULL;
	int new_topo = topology == NULL ? 1 : 0;

	if(new_topo){
		topology = hwloc_affinity_topology_load(NULL);
		if(topology == NULL)
			return NULL;
	}

	ldepth = hwloc_get_type_depth(topology, level);
	if(ldepth < 0){
		errno = EINVAL;
		goto out;
	}

	types = malloc((ldepth+2) * sizeof(*types));
	if (types == NULL) {
		errno = ENOMEM;
		goto out;
	}

	for (i = 0; i <= ldepth; i++) {
		obj = hwloc_get_obj_by_depth(topology, scatter ? i : ldepth-i, 0);
		types[i] = obj->type; 
	}
	if(level != HWLOC_OBJ_PU){
		types[ldepth+1] = HWLOC_OBJ_PU;
		ldepth++;
	}

	it = hwloc_tleaf_iterator_alloc(topology, ldepth+1, types);
	free(types);
	e = hwloc_tleaf_iterator_enumeration(it);
	hwloc_tleaf_iterator_free(it);
	
 out:
	if(new_topo)
		hwloc_topology_destroy(topology);
	return e;
}

struct cpuaffinity_enum *
cpuaffinity_scatter(hwloc_topology_t topology,
		  const hwloc_obj_type_t level)
{
	return cpuaffinity_default_policy(topology, level, 1);
}

struct cpuaffinity_enum *
cpuaffinity_round_robin(hwloc_topology_t topology,
		  const hwloc_obj_type_t level)
{
	return cpuaffinity_default_policy(topology, level, 0);
}
