/*
 * Copyright © 2010 INRIA
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

/* check a distance index array and matrix and insert it in the topology.
 * the caller gives us those pointers, we take care of freeing them later and so on.
 */
static int hwloc_topology__set_distance_matrix(hwloc_topology_t __hwloc_restrict topology, hwloc_obj_type_t type,
					       unsigned nbobjs, unsigned *indexes, unsigned *distances)
{
  unsigned i,j;

  /* make sure we don't have the same index twice */
  for(i=0; i<nbobjs; i++)
    for(j=i+1; j<nbobjs; j++)
      if (indexes[i] == indexes[j]) {
	free(indexes);
	free(distances);
	errno = EINVAL;
	return -1;
      }

  free(topology->os_distances[type].indexes);
  free(topology->os_distances[type].distances);
  topology->os_distances[type].nbobjs = nbobjs;
  topology->os_distances[type].indexes = indexes;
  topology->os_distances[type].distances = distances;
  return 0;
}

static hwloc_obj_t hwloc_find_obj_by_type_and_os_index(hwloc_obj_t root, hwloc_obj_type_t type, unsigned os_index)
{
  hwloc_obj_t child;
  if (root->type == type && root->os_index == os_index)
    return root;
  child = root->first_child;
  while (child) {
    hwloc_obj_t found = hwloc_find_obj_by_type_and_os_index(child, type, os_index);
    if (found)
      return found;
    child = child->next_sibling;
  }
  return NULL;
}

static void hwloc_get_type_distances_from_string(struct hwloc_topology *topology,
						 hwloc_obj_type_t type, char *string)
{
  /* the string format is: "index[0],...,index[N-1]:distance[0],...,distance[N*N-1]"
   * or "index[0],...,index[N-1]:X*Y" or "index[0],...,index[N-1]:X*Y*Z"
   */
  char *tmp = string, *next;
  unsigned *indexes;
  unsigned *distances;
  unsigned nbobjs = 0, i, j, x, y, z;

  /* count indexes */
  while (1) {
    size_t size = strspn(tmp, "0123456789");
    if (tmp[size] != ',') {
      /* last element */
      tmp += size;
      nbobjs++;
      break;
    }
    /* another index */
    tmp += size+1;
    nbobjs++;
  }

  if (*tmp != ':') {
    fprintf(stderr, "Ignoring %s distances from environment variable, missing colon\n",
	    hwloc_obj_type_string(type));
    return;
  }

  indexes = calloc(nbobjs, sizeof(unsigned));
  distances = calloc(nbobjs*nbobjs, sizeof(unsigned));
  tmp = string;

  /* parse indexes */
  for(i=0; i<nbobjs; i++) {
    indexes[i] = strtoul(tmp, &next, 0);
    tmp = next+1;
  }

  /* parse distances */
  z=1; /* default if sscanf finds only 2 values below */
  if (sscanf(tmp, "%u*%u*%u", &x, &y, &z) >= 2) {
    /* generate the matrix to create x groups of y elements */
    if (x*y*z != nbobjs) {
      fprintf(stderr, "Ignoring %s distances from environment variable, invalid grouping (%u*%u*%u=%u instead of %u)\n",
	      hwloc_obj_type_string(type), x, y, z, x*y*z, nbobjs);
      free(indexes);
      free(distances);
      return;
    }
    for(i=0; i<nbobjs; i++)
      for(j=0; j<nbobjs; j++)
	if (i==j)
	  distances[i*nbobjs+j] = 1;
	else if (i/z == j/z)
	  distances[i*nbobjs+j] = 2;
	else if (i/z/y == j/z/y)
	  distances[i*nbobjs+j] = 4;
	else
	  distances[i*nbobjs+j] = 8;

  } else {
    /* parse a comma separated list of distances */
    for(i=0; i<nbobjs*nbobjs; i++) {
      distances[i] = strtoul(tmp, &next, 0);
      tmp = next+1;
      if (!*next && i!=nbobjs*nbobjs-1) {
	fprintf(stderr, "Ignoring %s distances from environment variable, not enough values (%u out of %u)\n",
		hwloc_obj_type_string(type), i+1, nbobjs*nbobjs);
	free(indexes);
	free(distances);
	return;
      }
    }
  }

  if (hwloc_topology__set_distance_matrix(topology, type, nbobjs, indexes, distances) < 0)
    fprintf(stderr, "Ignoring invalid %s distances from environment variable\n", hwloc_obj_type_string(type));
}

/* take distances in the environment, store them as is in the topology.
 * we'll convert them into object later once the tree is filled
 */
void hwloc_store_distances_from_env(struct hwloc_topology *topology)
{
  hwloc_obj_type_t type;
  for(type = HWLOC_OBJ_SYSTEM; type < HWLOC_OBJ_TYPE_MAX; type++) {
    char envname[64];
    snprintf(envname, sizeof(envname), "HWLOC_%s_DISTANCES", hwloc_obj_type_string(type));
    char *env = getenv(envname);
    if (env)
      hwloc_get_type_distances_from_string(topology, type, env);
  }
}

/* take the given distance, store them as is in the topology.
 * we'll convert them into object later once the tree is filled.
 */
int hwloc_topology_set_distance_matrix(hwloc_topology_t __hwloc_restrict topology, hwloc_obj_type_t type,
				       unsigned nbobjs, unsigned *indexes, unsigned *distances)
{
  unsigned *_indexes, *_distances;

  /* copy the input arrays and give them to the topology */
  _indexes = malloc(nbobjs*sizeof(unsigned));
  memcpy(_indexes, indexes, nbobjs*sizeof(unsigned));
  _distances = malloc(nbobjs*nbobjs*sizeof(unsigned));
  memcpy(_distances, distances, nbobjs*nbobjs*sizeof(unsigned));
  return hwloc_topology__set_distance_matrix(topology, type, nbobjs, _indexes, _distances);
}

/* convert distance indexes that were previously stored in the topology
 * into actual objects.
 */
void hwloc_convert_distances_indexes_into_objects(struct hwloc_topology *topology)
{
  hwloc_obj_type_t type;
  for(type = HWLOC_OBJ_SYSTEM; type < HWLOC_OBJ_TYPE_MAX; type++) {
    unsigned nbobjs = topology->os_distances[type].nbobjs;
    unsigned *indexes = topology->os_distances[type].indexes;
    unsigned i;
    if (indexes) {
      hwloc_obj_t *objs = calloc(nbobjs, sizeof(hwloc_obj_t));
      /* traverse the topology and look for the relevant objects */
      for(i=0; i<nbobjs; i++) {
	hwloc_obj_t obj = hwloc_find_obj_by_type_and_os_index(topology->levels[0][0], type, indexes[i]);
	if (!obj) {
	  fprintf(stderr, "Ignoring %s distances from environment variable, unknown OS index %u\n",
		  hwloc_obj_type_string(type), indexes[i]);
	  free(objs);
	  free(topology->os_distances[type].indexes);
	  topology->os_distances[type].indexes = NULL;
	  free(topology->os_distances[type].distances);
	  topology->os_distances[type].distances = NULL;
	  continue;
	}
	objs[i] = obj;
      }
      /* we found objects, we don't need indexes anymore */
      topology->os_distances[type].objs = objs;
      free(topology->os_distances[type].indexes);
      topology->os_distances[type].indexes = NULL;
    }
  }
}

static void
hwloc_setup_distances_from_os_matrix(struct hwloc_topology *topology,
				     unsigned nbobjs,
				     hwloc_obj_t *objs, unsigned *osmatrix)
{
  unsigned i, j, li, lj, minl;
  unsigned min = UINT_MAX, max = 0;
  hwloc_obj_t root;
  float *matrix;
  hwloc_cpuset_t set;
  unsigned relative_depth;
  int idx;

  /* find the root */
  set = hwloc_bitmap_alloc();
  for(i=0; i<nbobjs; i++)
    hwloc_bitmap_or(set, set, objs[i]->cpuset);
  root = hwloc_get_obj_covering_cpuset(topology, set);
  assert(root);
  if (!hwloc_bitmap_isequal(set, root->cpuset)) {
    /* partial distance matrix not including all the children of a single object */
    hwloc_bitmap_free(set);
    return;
  }
  hwloc_bitmap_free(set);
  relative_depth = objs[0]->depth - root->depth; /* FIXME: what if the depth isn't always the same? */

  /* get the logical index offset, it's the min of all logical indexes */
  minl = UINT_MAX;
  for(i=0; i<nbobjs; i++)
    if (minl > objs[i]->logical_index)
      minl = objs[i]->logical_index;

  /* compute/check min/max values */
  for(i=0; i<nbobjs; i++)
    for(j=0; j<nbobjs; j++) {
      unsigned val = osmatrix[i*nbobjs+j];
      if (val < min)
	min = val;
      if (val > max)
	max = val;
    }
  if (!min) {
    /* Linux up to 2.6.36 reports ACPI SLIT distances, which should be memory latencies.
     * Except of SGI IP27 (SGI Origin 200/2000 with MIPS processors) where the distances
     * are the number of hops between routers.
     */
    hwloc_debug("%s", "minimal distance is 0, matrix does not seem to contain latencies, ignoring\n");
    return;
  }

  /* store the normalized latency matrix in the root object */
  idx = root->distances_count++;
  root->distances = realloc(root->distances, root->distances_count * sizeof(struct hwloc_distances_s));
  root->distances[idx].relative_depth = relative_depth;
  root->distances[idx].nbobjs = nbobjs;
  root->distances[idx].latency = matrix = malloc(nbobjs*nbobjs*sizeof(float));

  root->distances[0].latency_base = (float) min;
#define NORMALIZE_LATENCY(d) (((float) d)/(float) min)
  root->distances[0].latency_max = NORMALIZE_LATENCY(max);
  for(i=0; i<nbobjs; i++) {
    li = objs[i]->logical_index - minl;
    matrix[li*nbobjs+li] = NORMALIZE_LATENCY(osmatrix[i*nbobjs+i]);
    for(j=i+1; j<nbobjs; j++) {
      lj = objs[j]->logical_index - minl;
      matrix[li*nbobjs+lj] = NORMALIZE_LATENCY(osmatrix[i*nbobjs+j]);
      matrix[lj*nbobjs+li] = NORMALIZE_LATENCY(osmatrix[j*nbobjs+i]);
    }
  }
}

/* convert internal distances into logically-ordered distances
 * that can be exposed in the API
 */
void
hwloc_finalize_logical_distances(struct hwloc_topology *topology)
{
  unsigned nbobjs;
  hwloc_obj_type_t type;
  int depth;

  for (type = HWLOC_OBJ_SYSTEM; type < HWLOC_OBJ_TYPE_MAX; type++) {
    nbobjs = topology->os_distances[type].nbobjs;
    if (!nbobjs)
      continue;

    depth = hwloc_get_type_depth(topology, type);
    if (depth == HWLOC_TYPE_DEPTH_UNKNOWN || depth == HWLOC_TYPE_DEPTH_MULTIPLE)
      continue;

    assert(!!topology->os_distances[type].distances == !!topology->os_distances[type].objs);

    if (topology->os_distances[type].distances) {
      hwloc_obj_t root = topology->levels[0][0];
      assert(!root->distances_count);
      assert(!root->distances);

      hwloc_setup_distances_from_os_matrix(topology, nbobjs,
					   topology->os_distances[type].objs,
					   topology->os_distances[type].distances);

      /* clear things that are now unused */
      free(topology->os_distances[type].distances);
      free(topology->os_distances[type].objs);
      topology->os_distances[type].distances = NULL;
      topology->os_distances[type].objs = NULL;
      topology->os_distances[type].nbobjs = 0;
    }
  }
}
