/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
 * See COPYING in top-level directory.
****************************************************************************/

#include <time.h>
#include <stdlib.h>
#include "private/autogen/config.h"
#include "hwloc.h"

struct hwloc_distrib_level{
	hwloc_obj_type_t type; // level type.
	unsigned depth; // level depth.
        unsigned user_index; // Index of this level as provided by user order.
        unsigned arity; // Number of children of this level below parent.
	unsigned coord; // The current level object index [0..arity[.
	// Iteration order of this level objects. index[coord] give logical_index below parent.
	unsigned *index;
};

struct hwloc_distrib_iterator{
        hwloc_obj_t *roots;
        unsigned n_roots;
	unsigned root_coord;
	struct hwloc_distrib_level ** levels; // n_roots * n_levels
	unsigned n_levels;
};

static unsigned* range(const unsigned n){
	unsigned i,*r = malloc(n*sizeof(*r));

	if(r==NULL)
		return NULL;
	for(i=0; i<n; i++)
		r[i] = i;
	return r;
}

static unsigned* reversed_range(const unsigned n){
	unsigned i,*r = malloc(n*sizeof(*r));
	
	if(r==NULL)
		return NULL;
	for(i=0; i<n; i++){ r[i] = n-i-1; }
	return r;
}

static unsigned* shuffled_range(const unsigned n){
	unsigned i, *index, *ret, val;

	if ((index = range(n)) == NULL)
		return NULL;

	ret = malloc(n*sizeof(*ret));
	if (ret == NULL) {
		free(index);
		return NULL;
	}
  
	srand(time(NULL));  
	for(i=n;i>0;i--){
		val = rand()%(i);
		ret[i-1] = index[val];
		index[val] = index[i-1];
	}
	free(index);
	return ret;
}

static int hwloc_distrib_level_cmp_depth(const void *la, const void* lb){
	const struct hwloc_distrib_level *a = (struct hwloc_distrib_level *)la;	
	const struct hwloc_distrib_level *b = (struct hwloc_distrib_level *)lb;
	if(a->depth > b->depth)
		return 1;
	if(a->depth < b->depth)
		return -1;
	return 0;
}

static int hwloc_distrib_level_cmp_user_index(const void *la, const void* lb){
	const struct hwloc_distrib_level *a = (struct hwloc_distrib_level *)la;	
	const struct hwloc_distrib_level *b = (struct hwloc_distrib_level *)lb;
	if(a->user_index > b->user_index)
		return 1;
	if(a->user_index < b->user_index)
		return -1;
	return 0;
}

static struct hwloc_distrib_level *
hwloc_distrib_root_levels(hwloc_topology_t topology,
			  const hwloc_obj_t root,
			  const hwloc_obj_type_t *types,
			  const unsigned n_types,
			  const unsigned long flags)
{
	unsigned i;
        unsigned arity;
	hwloc_obj_t parent;
	struct hwloc_distrib_level *levels;

	levels = malloc(n_types * sizeof(*levels));
	if(levels == NULL)
		return NULL;
	
	for (i=0; i<n_types; i++){
		levels[i].type = types[i];
		levels[i].depth = hwloc_get_type_depth(topology, types[i]);
		if (levels[i].depth < 0 ){
			fprintf(stderr, "Cannot build iterator with objects %s of negative depth.\n", hwloc_obj_type_string(types[i]));
			goto failure;
		}
		levels[i].index = NULL;
		levels[i].coord = 0;
		levels[i].user_index = i;
		levels[i].arity = 0; // not set
	}

	// Sort by depth to compute arities.
	qsort(levels, n_types, sizeof(*levels), hwloc_distrib_level_cmp_depth);	
	
	// Walk from top to bottom and set arity to the maximum arity below root field.
	parent=root;	
	for (i=0; i<n_types; i++){
		while (parent) {
			arity = hwloc_get_nbobjs_inside_cpuset_by_depth(topology, parent->cpuset, levels[i].depth);
			levels[i].arity = arity > levels[i].arity ? arity : levels[i].arity;
			parent = hwloc_get_next_obj_inside_cpuset_by_depth(topology, root->cpuset, parent->depth, parent);
		}
		
		if (levels[i].arity == 0) {
			fprintf(stderr, "No object of type %s below level %s.\n", hwloc_obj_type_string(levels[i].type), hwloc_obj_type_string(levels[i-1].type));
			goto failure;
		}

		parent = hwloc_get_obj_inside_cpuset_by_depth(topology, root->cpuset, levels[i].depth, 0);
	}

	// Allocate levels index.
	for (i=0; i<n_types; i++){
		if (flags & HWLOC_DISTRIB_FLAG_SHUFFLE) {
			levels[i].index = shuffled_range(levels[i].arity);
		}
		else if (flags & HWLOC_DISTRIB_FLAG_REVERSE) {
			levels[i].index = reversed_range(levels[i].arity);
		}
		else {
			levels[i].index = range(levels[i].arity);
		}
		if (levels[i].index == NULL) {
			while(i--){ free(levels[i].index); }
			goto failure;
		}
	}

	return levels;

 failure:
	free(levels);
	return NULL;
}

static void hwloc_distrib_destroy_level(struct hwloc_distrib_level *levels){
	free(levels->index);
	free(levels);
}


struct hwloc_distrib_iterator *
hwloc_distrib_build_iterator(hwloc_topology_t topology,
			     hwloc_obj_t *roots,
			     const unsigned n_roots,
			     const hwloc_obj_type_t *levels,
			     const unsigned n_levels,
			     const unsigned long flags){
	unsigned i;
	struct hwloc_distrib_iterator *it = malloc(sizeof(*it) + sizeof(*it->levels) * n_roots);
	if(it == NULL)
		return NULL;

	it->roots = roots;
	it->n_roots = n_roots;
	it->root_coord = 0;
	it->n_levels = n_levels;
	it->levels = (struct hwloc_distrib_level **)((char*)it + sizeof(*it));

	for(i=0; i<n_roots; i++){
		it->levels[i] = hwloc_distrib_root_levels(topology, roots[i], levels, n_levels, flags);
		if(it->levels[i] == NULL){
			while(i--)
				hwloc_distrib_destroy_level(it->levels[i]);
			goto failure;
		}
	}

	return it;
	
 failure:
	free(it);
	return NULL;
}

HWLOC_DECLSPEC struct hwloc_distrib_iterator *
hwloc_distrib_iterator_round_robin(hwloc_topology_t topology,
				   const hwloc_obj_type_t type,
				   const unsigned long flags){
	hwloc_obj_t root = hwloc_get_obj_by_depth(topology, 0, 0);
	struct hwloc_distrib_iterator *it;

	it = malloc(sizeof(*it) + sizeof(hwloc_obj_t) + sizeof(struct hwloc_distrib_level*));	
	if(it == NULL)
		return NULL;

	it->roots = (hwloc_obj_t*) ((char*)it + sizeof(*it));
	*it->roots = root;	
	it->n_roots = 1;
	it->root_coord = 0;
	it->n_levels = 1;
	it->levels = (struct hwloc_distrib_level **)((char*)it + sizeof(*it) + sizeof(hwloc_obj_t));
        *it->levels = hwloc_distrib_root_levels(topology, root, &type, 1, flags);

	if (*it->levels == NULL){ free(it); return NULL; }
	return it;
}

HWLOC_DECLSPEC struct hwloc_distrib_iterator *
hwloc_distrib_iterator_scatter(hwloc_topology_t topology,
			       const hwloc_obj_type_t type,
			       const unsigned long flags){

	unsigned i=0, n=0;
	hwloc_obj_t obj, root = hwloc_get_obj_by_depth(topology, 0, 0);
	hwloc_obj_type_t *levels;
	struct hwloc_distrib_iterator *it;

	// Count depths with a non empty cpuset. 
	obj = root;
	while(obj){
		if ((obj->cpuset != NULL && !hwloc_bitmap_iszero(obj->cpuset)) && hwloc_get_type_depth(topology, obj->type) >= 0)
			n++;
		if (obj->type == type)
			break;
		obj = obj->first_child;
	}
	
	// fill levels array.
        levels = malloc(sizeof(*levels) * n);
	obj = root;
	while(obj){
		if( obj->cpuset != NULL && !hwloc_bitmap_iszero(obj->cpuset) && hwloc_get_type_depth(topology, obj->type) >= 0){
				levels[n-1-i] = obj->type;
				i++;
		}
		if (obj->type == type)
			break;		
		obj = obj->first_child;
	}
	
	it = malloc(sizeof(*it) + sizeof(hwloc_obj_t) + sizeof(struct hwloc_distrib_level*));
	
	if(it == NULL)
		goto failure;

	it->roots = (hwloc_obj_t*) ((char*)it + sizeof(*it));
	*it->roots = root;	
	it->n_roots = 1;
	it->root_coord = 0;
	it->n_levels = n;
	it->levels = (struct hwloc_distrib_level **)((char*)it + sizeof(*it) + sizeof(hwloc_obj_t));
	
        *it->levels = hwloc_distrib_root_levels(topology, root, levels, n, flags);

	if (*it->levels == NULL)
		goto failure_with_it;

	free(levels);
	return it;

 failure_with_it:
	free(it);
 failure:
	free(levels);
	return NULL;
}

void hwloc_distrib_destroy_iterator(struct hwloc_distrib_iterator *it){
	unsigned i;
	
	for(i=0; i<it->n_roots; i++)
		hwloc_distrib_destroy_level(it->levels[i]);
	free(it);
}

// Increment coordinates by one. Return 1 if iterator reached end and reset it.
// Else return 0.
static int
hwloc_distrib_iterator_inc(struct hwloc_distrib_iterator *it){
        int i;
	struct hwloc_distrib_level *levels;

 do_root:
	// Sort by user_index to increment coordinates.
	levels = it->levels[it->root_coord];
	qsort(levels, it->n_levels, sizeof(*levels), hwloc_distrib_level_cmp_user_index);
		
	for (i=it->n_levels-1; i>=0; i--){
		if(++levels[i].coord >= levels[i].arity)
			levels[i].coord = 0;
		else
			break;
	}
	if(i < 0 && levels[0].coord == 0){
		if (++it->root_coord == it->n_roots){
			it->root_coord = 0;
			return 0;
		} else {
			goto do_root;
		}
	}
	return 1;
}

int
hwloc_distrib_iterator_next(hwloc_topology_t topology,
			    struct hwloc_distrib_iterator *it,
			    hwloc_obj_t *next){
	unsigned i;
        struct hwloc_distrib_level *levels = it->levels[it->root_coord];
	hwloc_obj_t obj = it->roots[it->root_coord];
	unsigned coord;
	
	// Sort by depth to walk objects at set coordinates.
	qsort(levels, it->n_levels, sizeof(*levels), hwloc_distrib_level_cmp_depth);	
	
	for(i=0; i<it->n_levels; i++){
		coord = levels[i].index[levels[i].coord];
		obj = hwloc_get_obj_inside_cpuset_by_depth(topology, obj->cpuset, levels[i].depth, coord);
		if( obj == NULL)
			return hwloc_distrib_iterator_inc(it) && hwloc_distrib_iterator_next(topology, it, next);
				     
	}

	*next = obj;
        return hwloc_distrib_iterator_inc(it);
}
