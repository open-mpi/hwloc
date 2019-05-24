/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
****************************************************************************/

#include "private/autogen/config.h"
#include "hwloc-tbind.h"
#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

static hwloc_topology_t system_topology;

static hwloc_topology_t hwloc_test_topology_load(const char *file)
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

	hwloc_topology_set_all_types_filter(topology,
					    HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
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

static void test_enum(hwloc_topology_t topology)
{
	unsigned i;
	hwloc_obj_t obj, obj_e;
	struct cpuaffinity_enum *it, *it_e;

	it_e = cpuaffinity_enum_alloc(topology);
	it = cpuaffinity_round_robin(topology, HWLOC_OBJ_PU);
	assert(it != NULL);
	
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		assert(cpuaffinity_enum_append(it_e, obj) == 0);
	}
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		obj_e = cpuaffinity_enum_next(it_e);
		assert(obj == obj_e);
	}

	cpuaffinity_enum_free(it);
	cpuaffinity_enum_free(it_e);
}

/***************************************************************************/
/*                             Check tleaf policies                        */
/***************************************************************************/

static void test_round_robin(hwloc_topology_t topology)
{
	size_t i, ncore = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	hwloc_obj_t Core;
	hwloc_obj_t it_PU;
	struct cpuaffinity_enum * it;
	it = cpuaffinity_round_robin(topology, HWLOC_OBJ_CORE);
	assert(it != NULL);

	for(i=0; i<cpuaffinity_enum_size(it); i++){
		it_PU = cpuaffinity_enum_next(it);
		Core = hwloc_get_obj_by_type(topology,
					     HWLOC_OBJ_CORE,
					     i%ncore);
		Core = Core->children[i/ncore];
		assert(it_PU->type == Core->type);
		assert(it_PU->logical_index == Core->logical_index);
	}
	
	cpuaffinity_enum_free(it);
}

static void test_scatter(hwloc_topology_t topology)
{
	hwloc_obj_t obj, it_PU;
	ssize_t i, j, r, n, c, val, depth, *arities;
	struct cpuaffinity_enum * it;
	
	it = cpuaffinity_scatter(topology, HWLOC_OBJ_PU);
	assert(it != NULL);
	depth = hwloc_topology_get_depth(topology);
	arities = malloc(depth * sizeof(*arities));
	
	for (i = 0; i < depth-1; i++) {
		obj = hwloc_get_obj_by_depth(topology, i, 0);
		arities[i] = obj->arity;
	}

	for (i = 0; i < (int)cpuaffinity_enum_size(it); i++) {
		it_PU = cpuaffinity_enum_next(it);
		c = i;
		n = cpuaffinity_enum_size(it);
		val = 0;
		for (j = 0; j < depth - 1; j++) {
			r = c % arities[j];
			n = n / arities[j];
			c = c / arities[j];
			val += n * r;
		}
		assert(it_PU->logical_index == val);
	}

	free(arities);
	cpuaffinity_enum_free(it);
}

static int is_tleaf(hwloc_topology_t topology)
{
	hwloc_obj_t next, obj = hwloc_get_obj_by_depth(topology, 0, 0);
	while(obj){
		next = obj->next_cousin;
		while(next){
			if(next->arity != obj->arity)
				return 0;
			next = next->next_cousin;
		}
		obj = obj->first_child;
	}
	return 1;
}

static void test_topology(hwloc_topology_t topology)
{
	test_enum(topology);
	if(hwloc_get_type_depth(topology, HWLOC_OBJ_CORE) > 0)
		test_round_robin(topology);
	if(is_tleaf(topology) && hwloc_get_type_depth(topology, HWLOC_OBJ_PU) > 0)
		test_scatter(topology);
}

/***************************************************************************/
/*                     Check binding inside this process                   */
/***************************************************************************/

#if defined(_OPENMP) || defined(HAVE_PTHREAD)
static void * bind_thread(void * arg)
{
	pid_t pid = getpid();
	struct cpuaffinity_enum *e = arg;
	hwloc_obj_t obj = cpuaffinity_bind_thread(e, pid);
	return (void*) cpuaffinity_check(system_topology, obj, pid);
}
#endif

#ifdef _OPENMP
#include <omp.h>

static void test_openmp(void)
{
	struct cpuaffinity_enum *e;
	unsigned num_threads;
	int err = 0;
	
	e = cpuaffinity_round_robin(system_topology, HWLOC_OBJ_PU);
	assert(e != NULL);
		
	num_threads = hwloc_get_nbobjs_by_type(system_topology, HWLOC_OBJ_PU);
#pragma omp parallel num_threads(num_threads) shared(err)
	if(!bind_thread(e)) {err++;}
	assert(err == 0);
	cpuaffinity_enum_free(e);
}
#endif //_OPENMP

#ifdef HAVE_PTHREAD
#include <pthread.h>

static void test_pthreads(void)
{
	struct cpuaffinity_enum *e;
	unsigned i, num_threads;
	pthread_t *tids;
	void *ret;
	int err = 0;
	
	num_threads = hwloc_get_nbobjs_by_type(system_topology, HWLOC_OBJ_PU);
	e = cpuaffinity_round_robin(system_topology, HWLOC_OBJ_CORE);
	assert(e!= NULL);
	tids = malloc(num_threads * sizeof(*tids));

	for(i=0; i<num_threads; i++)
	        assert(pthread_create(tids+i, NULL, bind_thread, e) == 0);
	for(i=0; i<num_threads; i++){
		assert(pthread_join(tids[i], &ret) == 0);
		if(!ret) err++;
	}
	
	assert(err == 0);
	cpuaffinity_enum_free(e);
	free(tids);
}
#endif //HAVE_PTHREAD

/***************************************************************************/
/*                      Check attaching to this process                    */
/***************************************************************************/

#ifdef HWLOC_HAVE_PTRACE
#ifdef HAVE_PTHREAD
struct thread_arg{
	struct cpuaffinity_enum *e;
	int tid;
	pthread_t id;
};

static void * check_pthread(void * arg)
{
	struct thread_arg *targ = arg;
	hwloc_obj_t obj = cpuaffinity_enum_get(targ->e, targ->tid);
	return (void*) (intptr_t)cpuaffinity_check(system_topology, obj, getpid());
}

static void run_parallel_pthread_test(struct cpuaffinity_enum *e, const unsigned num_threads)
{
	unsigned i;
        struct thread_arg *tids;
	void *ret;
	int err = 0;
	
	tids = malloc(num_threads * sizeof(*tids));
	assert(tids);
	
	for(i=0; i<num_threads; i++){
		tids[i].e = e;
		tids[i].tid = i;
	        assert(pthread_create(&(tids[i].id), NULL, check_pthread, tids+i) == 0);
	}
	for(i=0; i<num_threads; i++){
		assert(pthread_join(tids[i].id, &ret) == 0);
		if(!ret) err++;
	}
	free(tids);
	assert(err == 0);
}
#endif //HAVE_PTHREAD


#ifdef _OPENMP
static void run_parallel_openmp_test(struct cpuaffinity_enum *e,
				     const unsigned num_threads)
{
	int err = 0;	
#pragma omp parallel num_threads(num_threads) shared(err)	
	{
		int tid = omp_get_thread_num();
		fprintf(stderr, "In child %d\n", tid);			
		hwloc_obj_t obj = cpuaffinity_enum_get(e, tid);
		if(!cpuaffinity_check(system_topology, obj, getpid())) err++;
	}
        assert(err == 0);
}
#endif //_OPENMP

#if defined(_OPENMP) || defined(HAVE_PTHREAD)
static void check_attach(void(* fn)(struct cpuaffinity_enum *, const unsigned))
{
	struct cpuaffinity_enum *e;
	unsigned num_threads;

	num_threads = hwloc_get_nbobjs_by_type(system_topology, HWLOC_OBJ_PU);
	e = cpuaffinity_round_robin(system_topology, HWLOC_OBJ_CORE);
	if(cpuaffinity_attach(e, getpid()) == 0)
		fn(e, num_threads);
	cpuaffinity_enum_free(e);
}
#endif //defined(_OPENMP) || defined(HAVE_PTHREAD)
#endif //HWLOC_HAVE_PTRACE

/***************************************************************************/
/*                         Check sequential binding                        */
/***************************************************************************/

static void test_self(void)
{
	struct cpuaffinity_enum *e;
	pid_t pid = getpid();
	hwloc_obj_t obj;
	size_t i;
	
	e = cpuaffinity_round_robin(system_topology, HWLOC_OBJ_PU);
	assert(e != NULL);

	for(i=0; i<cpuaffinity_enum_size(e); i++){
		obj = cpuaffinity_bind_thread(e, pid);
		assert(cpuaffinity_check(system_topology, obj, pid));
	}
	
	cpuaffinity_enum_free(e);
}


int main(void)
{
	hwloc_topology_t topology;
	system_topology = hwloc_test_topology_load(NULL);	
	assert(system_topology != NULL);

	const struct hwloc_topology_support * support =
		hwloc_topology_get_support(system_topology);
	test_topology(system_topology);

	if(support->cpubind &&
	   support->cpubind->set_thread_cpubind &&
	   support->cpubind->get_thread_cpubind){
		test_self();
#ifdef _OPENMP
		test_openmp();
#ifdef HWLOC_HAVE_PTRACE	
		check_attach(run_parallel_openmp_test);
#endif // HWLOC_HAVE_PTRACE
#endif // _OPENMP
#ifdef HAVE_PTHREAD
		test_pthreads();
#ifdef HWLOC_HAVE_PTRACE	
		check_attach(run_parallel_pthread_test);
#endif // HWLOC_HAVE_PTRACE	
#endif // HAVE_PTHREAD
	}
	hwloc_topology_destroy(system_topology);

	DIR* xml_dir = opendir(TBIND_XML_DIR);
	if(xml_dir == NULL){
		perror("opendir");
		return 1;
	}

	char fname[256];
	struct dirent *dirent;
	for(dirent = readdir(xml_dir);
	    dirent != NULL;
	    dirent = readdir(xml_dir)){
		// Not supported by solaris and not critical.
		/* if(dirent->d_type != DT_REG) */
		/* 	continue; */
		if(strcmp(dirent->d_name + strlen(dirent->d_name) - 4,
			  ".xml"))
			continue;
		memset(fname, 0, sizeof(fname));
		snprintf(fname,
			 sizeof(fname),
			 "%s/%s",
			 TBIND_XML_DIR,
			 dirent->d_name);
		topology = hwloc_test_topology_load(fname);
		if(topology == NULL)
			continue;
		test_topology(topology);
		hwloc_topology_destroy(topology);
	}
	
	closedir(xml_dir);
	return 0;
}

