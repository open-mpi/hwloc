#include "misc.h"
#include "hwloc-thread-bind.h"
#include "hwloc/helper.h"
#include "hwloc-calc.h"

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "private/autogen/config.h"
#if HWLOC_HAVE_PTRACE
#include <sys/ptrace.h>
#endif

extern int logical; // Whether indexing is logical
extern int verbose; // Whether to verbose print. 

/** Maximum len of string containing a hwloc_obj logical index **/
#define STR_OBJ_MAX 32

/**********************************************************************/
/*                           enum  structure                          */
/**********************************************************************/

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

struct cpuaffinity_enum *
cpuaffinity_enum_alloc(hwloc_topology_t topology)
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

void
cpuaffinity_enum_free(struct cpuaffinity_enum *obj)
{
	free(obj->obj);
	free(obj);
}

size_t
cpuaffinity_enum_size(struct cpuaffinity_enum *obj)
{
	return obj->n;
}

int
cpuaffinity_enum_append(struct cpuaffinity_enum *e, hwloc_obj_t obj)
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

hwloc_obj_t
cpuaffinity_enum_next(struct cpuaffinity_enum *e)
{
	hwloc_obj_t obj = e->obj[e->current];
	e->current = (e->current + 1) % e->n;
        return obj;
}

hwloc_obj_t
cpuaffinity_enum_get(struct cpuaffinity_enum * e,
		     const size_t i)
{
	if(e == NULL)
		return NULL;
	return e->obj[i % e->n];
}

static int cpuaffinity_obj_snprintf(char* str,
				    const size_t len,
				    const char *sep,
				    const hwloc_obj_t obj,
				    const int cpuset,
				    const int taskset)
{
	char* c = str;
	int index = logical ? obj->logical_index : obj->os_index;
	if(taskset)
		c+=hwloc_bitmap_taskset_snprintf(str, len, obj->cpuset);
	else if(cpuset)
		c+=hwloc_bitmap_snprintf(str, len, obj->cpuset);
	else
		c += snprintf(c, len, "%d", index);
	c += snprintf(c, c-str+len, "%s", sep);
	return c-str;
}

void cpuaffinity_enum_print(const struct cpuaffinity_enum *e,
			    const char *sep,
			    const int cpuset,
			    const int taskset,
			    const int reverse,
			    unsigned num)
{
	size_t len = e->n * (strlen(sep) + STR_OBJ_MAX);
	char *c, *enum_str = malloc(len);
	int i, start, end;
	num = num == 0 ? e->n : num;
	num = num > e->n ? e->n : num;

	if (enum_str == NULL) {
		errno = ENOMEM;
		return;
	}

	memset(enum_str, 0, len);
	c = enum_str;

	start = reverse ? num-1 : 0;
	end   = reverse ? 0     : num-1;
	for (i = start; i != end; reverse ? i-- : i++){
		c+=cpuaffinity_obj_snprintf(c,
					    len + enum_str - c,
					    sep,
					    e->obj[i],
					    cpuset,
					    taskset);
	}
	c+=cpuaffinity_obj_snprintf(c,
				    len + enum_str - c,
				    "",
				    e->obj[i],
				    cpuset,
				    taskset);
	printf("%s\n", enum_str);
	free(enum_str);
}

hwloc_obj_t
cpuaffinity_bind_thread(struct cpuaffinity_enum * objs,
			const pid_t tid)
{
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

#if HWLOC_HAVE_PTRACE
int
cpuaffinity_attach(const pid_t pid,
		   struct cpuaffinity_enum *e,
		   const int repeat,
		   const int stopped)
{
	const struct hwloc_topology_support * support =
		hwloc_topology_get_support(e->topology);
	if(!support->cpubind->set_thread_cpubind){
		fprintf(stderr,
			"cpuaffinity_attach: set_thread_cpubind not supported.\n");
		return -1;
	}

	/* Wait for child to stop */
	if(!stopped)
		kill(pid, SIGSTOP);
	waitpid(pid, NULL, WUNTRACED);
	
	/* attach and set options to trace threads creation and process exit */
	if(ptrace(PTRACE_SEIZE,
		  pid,
		  NULL,
		  (void*)(PTRACE_O_TRACECLONE|PTRACE_O_TRACEFORK)) == -1){
		perror("PTRACE_SEIZE");
		return -1;
	}

	/* Resume stopped child */
	kill(pid, SIGCONT);
	
	/* wait childrens until process exits */
	do{
		int status;
		pid_t child = waitpid(-1, &status, __WALL);
		if(WIFEXITED(status) && child == pid){ return WEXITSTATUS(status);}
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
					if(e->current < e->n || repeat){
						if(verbose){
							hwloc_obj_t loc = e->obj[e->current];
							printf("Binding thread %lu to %s:%d\n",
							       eventmsg,
							       hwloc_obj_type_string(loc->type),
							       logical ? loc->logical_index
							       : loc->os_index);
						}
						cpuaffinity_bind_thread(e, eventmsg);
					}
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
#endif // HWLOC_HAVE_PTRACE

hwloc_cpuset_t
hwloc_process_location(hwloc_topology_t topology,
		       const char* str)
{
	int err;
	hwloc_bitmap_t cpuset;
	
        cpuset = hwloc_bitmap_alloc();	
	if(cpuset == NULL){
		perror("hwloc_bitmap_alloc");
		exit(1);
	}	
	
	struct hwloc_calc_location_context_s lcontext = {
		.topology = topology,
		.topodepth = hwloc_topology_get_depth(topology),
		.only_hbm = -1,
		.logical = logical,
		.verbose = 0
	};
	struct hwloc_calc_set_context_s scontext = {
		.nodeset_input = 0,
		.nodeset_output = 0,
		.output_set = cpuset,
	};
	
	err = hwloc_calc_process_location_as_set(&lcontext,
						 &scontext,
						 str);

	if(err < 0){
		fprintf(stderr,
			"Obj %s is not recognized or does not contain a cpuset.\n",
			str);
	        return NULL;
	}

	return cpuset;
}

int
restrict_topology(hwloc_topology_t topology,
		  const char *restrict_str)
{
	int err = 0;
	hwloc_bitmap_t restrict_cpuset;
	
	restrict_cpuset = hwloc_process_location(topology, restrict_str);
	if(restrict_cpuset == NULL)
		return -1;

        hwloc_topology_restrict(topology,
				restrict_cpuset,
				HWLOC_RESTRICT_FLAG_REMOVE_CPULESS);
	if(err != 0)
		perror("hwloc_topology_restrict");

	hwloc_bitmap_free(restrict_cpuset);
	return err;			
}
