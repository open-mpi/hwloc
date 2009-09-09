/* topo-hello.c */
#include <hwloc.h>

static void print_children(topo_topology_t topology, topo_obj_t obj, int depth)
{
	char string[128];
	int i;

	topo_obj_snprintf(string, sizeof(string), topology, obj, "#", 0);
	printf("%*s%s\n", 2*depth, "", string);
	for (i = 0; i < obj->arity; i++)
		print_children(topology, obj->children[i], depth + 1);
}

int main(void)
{
	/* Topology object */
	topo_topology_t topology;

	/* Allocate and initialize topology object.  */
	topo_topology_init(&topology);

        /* ... Optionally, put detection configuration here to e.g. ignore some
           objects types, define a synthetic topology, etc....  The default is
           to detect all the objects of the machine that the caller is allowed
           to access.
	   See Configure Topology Detection.  */

	/* Perform the topology detection.  */
	topo_topology_load(topology);


	/* Optionally, get some additional topology information
	 * in case we need the topology depth later.
	 */
	struct topo_topology_info topoinfo;
	topo_topology_get_info(topology, &topoinfo);


        /* Walk the topology with an array style, from level 0 (always the
         * system level) to the lowest level (always the proc level). */
	int depth, i;
	char string[128];
	for (depth = 0; depth < topoinfo.depth; depth++) {
		for (i = 0; i < topo_get_depth_nbobjs(topology, depth); i++) {
			topo_obj_snprintf(string, sizeof(string), topology,
					topo_get_obj_by_depth(topology, depth, i), "#", 0);
			printf("%s\n", string);
		}
	}

	/* Walk the topology with a tree style.  */
	print_children(topology, topo_get_system_obj(topology), 0);


	/* Print the number of sockets.  */
	depth = topo_get_type_depth(topology, TOPO_OBJ_SOCKET);
	if (depth == TOPO_TYPE_DEPTH_UNKNOWN)
		printf("The number of sockets is unknown\n");
	else
		printf("%u socket(s)\n", topo_get_depth_nbobjs(topology, depth));


        /* Find out where cores are, or else smaller sets of CPUs if the OS
         * doesn't have the notion of core. */
	depth = topo_get_type_or_below_depth(topology, TOPO_OBJ_CORE);

	/* Get last one.  */
	topo_obj_t obj = topo_get_obj_by_depth(topology, depth, topo_get_depth_nbobjs(topology, depth) - 1);
	if (!obj)
		return 0;

	/* Get its cpuset.  */
	topo_cpuset_t cpuset = obj->cpuset;

	/* Get only one logical processor (in case the core is SMT/hyperthreaded).  */
	topo_cpuset_singlify(&cpuset);

	/* And try to bind ourself there.  */
	if (topo_set_cpubind(topology, &cpuset, 0)) {
		char s[TOPO_CPUSET_STRING_LENGTH + 1];
		topo_cpuset_snprintf(s, sizeof(s), &obj->cpuset);
		printf("Couldn't bind to cpuset %s\n", s);
	}


	/* Destroy topology object.  */
	topo_topology_destroy(topology);

	return 0;
}
