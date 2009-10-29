/* Example hwloc API program.
 *
 * Copyright © 2009 INRIA, Université Bordeaux 1
 *
 * topo-hello.c 
 */

#include <hwloc.h>

static void print_children(hwloc_topology_t topology, hwloc_obj_t obj, int depth)
{
	char string[128];
	int i;

	hwloc_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
	printf("%*s%s\n", 2*depth, "", string);
	for (i = 0; i < obj->arity; i++)
		print_children(topology, obj->children[i], depth + 1);
}

int main(void)
{
	/* Topology object */
	hwloc_topology_t topology;

	/* Allocate and initialize topology object.  */
	hwloc_topology_init(&topology);

        /* ... Optionally, put detection configuration here to e.g. ignore some
           objects types, define a synthetic topology, etc....  The default is
           to detect all the objects of the machine that the caller is allowed
           to access.
	   See Configure Topology Detection.  */

	/* Perform the topology detection.  */
	hwloc_topology_load(topology);


	/* Optionally, get some additional topology information
	 * in case we need the topology depth later.
	 */
	unsigned topodepth = hwloc_topology_get_depth(topology);


        /* Walk the topology with an array style, from level 0 (always the
         * system level) to the lowest level (always the proc level). */
	int depth, i;
	char string[128];
	for (depth = 0; depth < topodepth; depth++) {
		for (i = 0; i < hwloc_get_nbobjs_by_depth(topology, depth); i++) {
			hwloc_obj_snprintf(string, sizeof(string), topology,
					hwloc_get_obj_by_depth(topology, depth, i), "#", 0);
			printf("%s\n", string);
		}
	}

	/* Walk the topology with a tree style.  */
	print_children(topology, hwloc_get_system_obj(topology), 0);


	/* Print the number of sockets.  */
	depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
	if (depth == HWLOC_TYPE_DEPTH_UNKNOWN)
		printf("The number of sockets is unknown\n");
	else
		printf("%u socket(s)\n", hwloc_get_nbobjs_by_depth(topology, depth));


        /* Find out where cores are, or else smaller sets of CPUs if the OS
         * doesn't have the notion of core. */
	depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);

	/* Get last one.  */
	hwloc_obj_t obj = hwloc_get_obj_by_depth(topology, depth, hwloc_get_nbobjs_by_depth(topology, depth) - 1);
	if (!obj)
		return 0;

	/* Get a copy of its cpuset that we may modify.  */
	hwloc_cpuset_t cpuset = hwloc_cpuset_dup(obj->cpuset);

	/* Get only one logical processor (in case the core is SMT/hyperthreaded).  */
	hwloc_cpuset_singlify(cpuset);

	/* And try to bind ourself there.  */
	if (hwloc_set_cpubind(topology, cpuset, 0)) {
		char *str = NULL;
		hwloc_cpuset_asprintf(&str, obj->cpuset);
		printf("Couldn't bind to cpuset %s\n", str);
		free(str);
	}

	/* Free our cpuset copy */
	hwloc_cpuset_free(cpuset);

	/* Destroy topology object.  */
	hwloc_topology_destroy(topology);

	return 0;
}
