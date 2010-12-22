/*
 * Copyright © 2010 INRIA
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <hwloc.h>
#include <private/private.h>
#include <private/debug.h>

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
  hwloc_obj_t *objs;

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

  /* traverse the topology and look for the relevant objects */
  objs = calloc(nbobjs, sizeof(hwloc_obj_t));
  for(i=0; i<nbobjs; i++) {
    hwloc_obj_t obj = hwloc_find_obj_by_type_and_os_index(topology->levels[0][0], type, indexes[i]);
    if (!obj) {
      fprintf(stderr, "Ignoring %s distances from environment variable, unknown OS index %u\n",
	      hwloc_obj_type_string(type), indexes[i]);
      free(indexes);
      free(distances);
      free(objs);
      return;
    }
    objs[i] = obj;
  }

  free(indexes);
  topology->os_distances[type].nbobjs = nbobjs;
  topology->os_distances[type].distances = distances;
  topology->os_distances[type].objs = objs;
}

void hwloc_get_distances_from_env(struct hwloc_topology *topology)
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

static void
hwloc_setup_distances_from_os_matrix(struct hwloc_topology *topology,
				     hwloc_obj_t root, unsigned relative_depth, unsigned nbobjs,
				     hwloc_obj_t *objs, unsigned *osmatrix)
{
  unsigned i, j, li, lj;
  unsigned min = UINT_MAX, max = 0;
  float *matrix;
  int idx;

  /* check that root/depth/nbobjs are consistent */
  if (hwloc_get_nbobjs_inside_cpuset_by_depth(topology, root->cpuset, root->depth + relative_depth) != nbobjs)
    return;

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
    li = objs[i]->logical_index;
    matrix[li*nbobjs+li] = NORMALIZE_LATENCY(osmatrix[i*nbobjs+i]);
    for(j=i+1; j<nbobjs; j++) {
      lj = objs[j]->logical_index;
      matrix[li*nbobjs+lj] = NORMALIZE_LATENCY(osmatrix[i*nbobjs+j]);
      matrix[lj*nbobjs+li] = NORMALIZE_LATENCY(osmatrix[j*nbobjs+i]);
    }
  }
}

void
hwloc_set_logical_distances(struct hwloc_topology *topology)
{
  unsigned nbobjs;
  hwloc_obj_type_t type;
  unsigned depth;

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

      hwloc_setup_distances_from_os_matrix(topology, root, depth, nbobjs,
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
