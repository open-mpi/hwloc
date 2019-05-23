#include "private/autogen/config.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "hwloc-tbind.h"
#include "hwloc/helper.h"

/***************************************************************************/
/*                                   utils                                 */
/***************************************************************************/

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

/***************************************************************************/
/*                              enum  structure                            */
/***************************************************************************/

struct cpuaffinity_enum{
	/** The topology used to build enum **/
	hwloc_topology_t topology;
	/** Index of current hwloc_obj in enumeration **/
        unsigned current;
	/** Number of hwloc_obj in enumeration **/
	unsigned n;
	/** 
	 * Maximum number of hwloc_obj storable. 
	 * This the number of HWLOC_OBJ_PU in topology.
	 **/
	unsigned nmax;
	/** 
	 * Array of processing units.
	 * These are the pointers from
	 * an existing topology. Topology must not 
	 * be destroyed until the enum is.
	 **/
	hwloc_obj_t *obj;
};

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

/***************************************************************************/
/*                               Tleaf structure                           */
/***************************************************************************/

struct hwloc_tleaf_iterator {
	hwloc_topology_t topology;
	/** Actual number of levels in hwloc_tleaf_iterator **/
	int n;
	/** Topology depth of levels. **/
	unsigned *depth;
	/** Index for reordering levels in ascending order. **/
	unsigned *asc_order;
	/** Number of elements at level below parent. **/
	unsigned *arity;
	/** Current iterator index of level.w **/
	unsigned *it;
	/** For each level indexing of elements. **/
	unsigned **index;
};

static inline void shuffle_index(unsigned *x, const size_t n)
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

static inline unsigned max_arity(const struct hwloc_tleaf_iterator *t,
				    const unsigned parent_depth,
				    const unsigned child_depth)
{
	unsigned arity = 0, n = 0;
	hwloc_obj_t p = hwloc_get_obj_by_depth(t->topology, parent_depth, 0);
	while(p){
		n = hwloc_get_nbobjs_inside_cpuset_by_depth (t->topology,
							     p->cpuset,
							     child_depth);
		arity = n > arity ? n : arity;
		p = p->next_cousin;
	}
	return arity;
}

#define chk_ptr(ptr, goto_flag) \
	if(ptr == NULL){errno = ENOMEM; goto goto_flag;}

struct hwloc_tleaf_iterator *
hwloc_tleaf_iterator_alloc(hwloc_topology_t topology,
			   const int n_depths,
			   const int *depths)
{
	int i, n, cd, pd;
	struct hwloc_tleaf_iterator *t;

	t = malloc(sizeof(*t));
	if (t == NULL)
		return NULL;

	t->depth = malloc(n_depths * sizeof(*t->depth));
	chk_ptr(t->depth, error);

	t->asc_order = malloc(n_depths * sizeof(*t->asc_order));
	chk_ptr(t->asc_order, error_with_depth);

	t->arity = malloc(n_depths * sizeof(*t->arity));
	chk_ptr(t->arity, error_with_order);

	t->it = malloc(n_depths * sizeof(*t->it));
	chk_ptr(t->it, error_with_size);

	t->index = malloc(n_depths * sizeof(*t->index));
	chk_ptr(t->index, error_with_it);

        t->topology = topology;

	t->n = n_depths;

	// Assign depths and set iteration step
	for (i = 0; i < n_depths; i++) {
		if(depths[i] < 0){
			errno = EINVAL;
			goto error_with_index;
		}
		t->depth[i] = depths[i];
		t->it[i] = 0;
	}

	// order depths in asc_order
	memcpy(t->asc_order, t->depth, n_depths * sizeof(*t->asc_order));
	qsort(t->asc_order, n_depths, sizeof(*t->asc_order), comp_unsigned);

	// replace asc_order with index of matching element in depth.
	for (n = 0; n < t->n; n++)
		for (i = 0; i < t->n; i++)
			if (t->depth[i] == t->asc_order[n]) {
				t->asc_order[n] = i;
				break;
			}
	// Compute depths arity.
	for (i = 1; i < n_depths; i++) {
		pd = t->depth[t->asc_order[i]];
		cd = t->depth[t->asc_order[i - 1]];
		t->arity[t->asc_order[i - 1]] = max_arity(t, pd, cd);
	}
	t->arity[t->asc_order[t->n - 1]] =
	    hwloc_get_nbobjs_by_depth(t->topology,
				      t->depth[t->asc_order[t->n - 1]]);

	// Allocate iteration index for depths
	for (i = 0; i < n_depths; i++) {
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
		shuffle_index(t->index[i], t->arity[i]);
}

#define hwloc_get_depth_type(topology, depth)			\
	hwloc_get_obj_by_depth(topology, depth, 0)->type

hwloc_obj_t
hwloc_tleaf_iterator_current(const struct hwloc_tleaf_iterator *t)
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

	for (i = t->n - 2; i >= 0 && p != NULL; i--) {
		j = t->asc_order[i];
		p = hwloc_get_obj_below_by_type(t->topology,
						p->type,
						p->logical_index,
						hwloc_get_depth_type(t->topology,
								     t->depth[j]),
						t->index[j][t->it[j]]);
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

struct cpuaffinity_enum *
hwloc_tleaf_iterator_enumeration(struct hwloc_tleaf_iterator *it)
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
		  const size_t n_depths,
		  const int *depths,
		  const int shuffle)
{
	int new_topo = topology == NULL ? 1 : 0;
	struct hwloc_tleaf_iterator * it;
	struct cpuaffinity_enum * e;
	if(n_depths <= 1){
		errno = EINVAL;
		return NULL;
	}
		
	if(new_topo){
		topology = hwloc_affinity_topology_load(NULL);
		if(topology == NULL)
			return NULL;
	}
	it = hwloc_tleaf_iterator_alloc(topology, n_depths, depths);
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
	int *depths;
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

	depths = malloc((ldepth+2) * sizeof(*depths));
	if (depths == NULL) {
		errno = ENOMEM;
		goto out;
	}

	for (i = 0; i <= ldepth; i++) {
		obj = hwloc_get_obj_by_depth(topology,
					     scatter ? i : ldepth-i, 0);
	        depths[i] = obj->depth; 
	}
	if(level != HWLOC_OBJ_PU){
		depths[ldepth+1] = hwloc_get_type_depth(topology, HWLOC_OBJ_PU);
		ldepth++;
	}

	it = hwloc_tleaf_iterator_alloc(topology, ldepth+1, depths);
	free(depths);
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

/***************************************************************************/
/*                         Binding process threads                         */
/***************************************************************************/

int cpuaffinity_check(const hwloc_topology_t topology, const hwloc_obj_t target, const pid_t tid)
{
	int ret = 0;    
	hwloc_bitmap_t checkset = hwloc_bitmap_alloc();

	if(hwloc_get_proc_cpubind(topology,
				  tid,
				  checkset,
				  HWLOC_CPUBIND_THREAD) == -1){
		perror("get_cpubind");
		goto check_cpubind_exit;
	}

	hwloc_obj_t bound = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);

	while(bound != NULL &&
	      !hwloc_bitmap_isincluded(bound->cpuset, target->cpuset)){
		if(bound->next_cousin == NULL){
			bound = hwloc_get_obj_by_depth(topology,
						       bound->depth-1,
						       0);
		} else {
			bound = bound->next_cousin;
		}
	}
	while(bound->depth > target->depth){bound = bound->parent;}

	if(bound == NULL){
		fprintf(stderr, "Binding outside of topology\n");
		goto check_cpubind_exit;
	}
	if(bound != target){
		fprintf(stderr, "Binding on %s:%d instead of %s:%d\n",
			hwloc_obj_type_string(bound->type),
			bound->logical_index,
			hwloc_obj_type_string(target->type),
			target->logical_index);
	} else {
		ret = 1;
	}

 check_cpubind_exit:
	hwloc_bitmap_free(checkset);
	return ret;
}

hwloc_obj_t cpuaffinity_bind_thread(struct cpuaffinity_enum * objs,
				    const pid_t tid){
	hwloc_obj_t obj = cpuaffinity_enum_next(objs);

	if(hwloc_set_proc_cpubind(objs->topology,
				  tid,
				  obj->cpuset,
				  HWLOC_CPUBIND_THREAD) == -1){
		perror("hwloc_set_cpubind");
		return NULL;
	}
	return obj;
}

#ifdef HAVE_SYS_PTRACE_H
int cpuaffinity_attach(struct cpuaffinity_enum * objs, const pid_t pid)
{
	/*attach and set options to trace threads creation and process exit*/
	if(ptrace(PTRACE_SEIZE,
		  pid,
		  NULL,
		  (void*)(PTRACE_O_TRACECLONE|PTRACE_O_TRACEFORK)) == -1){
		perror("PTRACE_SEIZE"); return -1;
	}

	/* wait childrens until process exits */
	do{
		int status;
		pid_t child = waitpid(-1, &status, __WALL);
		if(WIFEXITED(status) && child == pid){break;}
		if(WIFSIGNALED(status) && child == pid){break;}
      
		/* Child Stopped */
		if(WIFSTOPPED(status)){
			int sig = WSTOPSIG(status);
			if(sig == SIGTRAP){
				/*Get ptrace event that triggered the stop*/
				int event = status >> 8;
				unsigned long eventmsg;
	  
				if(ptrace(PTRACE_GETEVENTMSG,
					  child,
					  NULL,
					  (void*)(&eventmsg)) == -1){
					perror("PTRACE_GETEVENTMSG");
					continue;
				}
				if(event == (SIGTRAP|(PTRACE_EVENT_FORK<<8))  ||
				   event == (SIGTRAP|(PTRACE_EVENT_VFORK<<8)) ||
				   event == (SIGTRAP|(PTRACE_EVENT_CLONE<<8))){
					cpuaffinity_bind_thread(objs,
								eventmsg);
				}
			}
			/* Resume stopped child */
			if(ptrace(PTRACE_CONT, child, NULL, NULL) == -1) {
				perror("PTRACE_CONT(interrupt)");
			}
		}
	} while(1);
	return 0;
}

pid_t cpuaffinity_exec(struct cpuaffinity_enum * objs, char** argv)
{
	pid_t pid = fork();  
	/* start program to trace */
	if(pid == 0) {
		if(execvp(argv[0], argv) == -1){
			perror("execvp");
			return -1;
		}
		return 0;
	}
	/* Tracer code */
	else if(pid > 0){
		cpuaffinity_attach(objs, pid);
		return pid;
	}
	/* fork error */
	else {
		perror("fork");
		return -1;
	} 
}
#endif //HAVE_SYS_PTRACE_H