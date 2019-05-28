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
#ifdef HWLOC_HAVE_SYS_GETTID
#include <sys/syscall.h>
#endif
#ifdef _OPENMP
#include<omp.h>
#endif
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

static inline hwloc_obj_t topology_leaf(hwloc_topology_t topology){
	int depth = hwloc_topology_get_depth(topology);
        return hwloc_get_obj_by_depth(topology, depth-1, 0);
}

static void test_enum(hwloc_topology_t topology)
{
	unsigned i;
	hwloc_obj_t obj, obj_e;
	struct cpuaffinity_enum *it, *it_e;
	hwloc_obj_t leaf = topology_leaf(topology);

	it_e = cpuaffinity_enum_alloc(topology);
	it = cpuaffinity_round_robin(topology, leaf);
	assert(it != NULL);
	
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		assert(cpuaffinity_enum_append(it_e, obj) == 0);
	}
	for (i = 0; i < cpuaffinity_enum_size(it); i++){
		obj = cpuaffinity_enum_next(it);
		obj_e = cpuaffinity_enum_next(it_e);
		assert(obj == obj_e);
		assert(obj->logical_index == i);
	}

	cpuaffinity_enum_free(it);
	cpuaffinity_enum_free(it_e);
}

/***************************************************************************/
/*                             Check tleaf policies                        */
/***************************************************************************/

static void test_round_robin(hwloc_topology_t topology)
{
	hwloc_obj_t leaf = topology_leaf(topology);
	hwloc_obj_t Core = leaf->parent;
	hwloc_obj_t obj;
	size_t i, n, ncore = hwloc_get_nbobjs_by_depth(topology, Core->depth);
	hwloc_obj_t it_PU;
	struct cpuaffinity_enum * it;	

	it = cpuaffinity_round_robin(topology, Core);
	assert(it != NULL);

	for(i=0; i<cpuaffinity_enum_size(it); i++){
		it_PU = cpuaffinity_enum_next(it);
	        obj = hwloc_get_obj_by_depth(topology,
					     Core->depth,
					     i%ncore);
		n = i/ncore;
		while(n >= obj->arity){
			obj = obj->next_cousin;
			assert(obj);
		}
	        obj = obj->children[n];
		assert(leaf->type == obj->type);
		assert(it_PU->type == obj->type);
		assert(it_PU->logical_index == obj->logical_index);
	}
	
	cpuaffinity_enum_free(it);
}

static void test_scatter(hwloc_topology_t topology)
{
	hwloc_obj_t obj, it_PU;
	ssize_t i, j, r, n, c, val, depth, *arities;
	struct cpuaffinity_enum * it;
	
	it = cpuaffinity_scatter(topology, topology_leaf(topology));
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

static void test_policies(hwloc_topology_t topology)
{
	test_enum(topology);
	test_round_robin(topology);
	if(is_tleaf(topology))
		test_scatter(topology);
}

/***************************************************************************/
/*                      Testing binding inside thread                      */
/***************************************************************************/
#ifdef HWLOC_HAVE_SYS_GETTID
#ifdef _OPENMP
static int check_strategy_openmp(struct cpuaffinity_enum *
				 (*strategy)(hwloc_topology_t,
					     const hwloc_obj_t),
				 int prebind)	
{
	hwloc_obj_t target;
	unsigned num_threads;
	int err = 0;
	struct cpuaffinity_enum * it;
	hwloc_obj_t leaf = topology_leaf(system_topology);
	
	num_threads = hwloc_get_nbobjs_by_type(system_topology, leaf->type);
	it = strategy(system_topology, leaf);
	if(it == NULL)
		return 0;
	
	if(prebind){
#pragma omp parallel num_threads(num_threads) shared(err, it) private(target)
		{
			pid_t pid = syscall(SYS_gettid);
			target = cpuaffinity_enum_get(it, omp_get_thread_num());
			hwloc_set_proc_cpubind(system_topology,
					       pid,
					       target->cpuset,
					       HWLOC_CPUBIND_THREAD);
			err += !cpuaffinity_check(system_topology, target, pid);
		}
		if(err != 0)
			goto out;
	}
		
#pragma omp parallel num_threads(num_threads) shared(it, err) private(target)
	{
		pid_t pid = syscall(SYS_gettid);
		target = cpuaffinity_enum_get(it, omp_get_thread_num());
		err += !cpuaffinity_check(system_topology, target, pid);
	}

 out:
	cpuaffinity_enum_free(it);
        return err == 0;
}
#endif

#ifdef HAVE_PTHREAD

struct pthread_arg {
	int prebind;
	hwloc_obj_t target;
};

static void* pthread_check(void* arg)
{
	int ret;
	int tid = syscall(SYS_gettid);
	struct pthread_arg *parg = arg;
	if(parg->prebind)
		hwloc_set_proc_cpubind(system_topology,
				       tid,
				       parg->target->cpuset,
				       HWLOC_CPUBIND_THREAD);
	ret = cpuaffinity_check(system_topology, parg->target, tid);
	return (void*)(intptr_t)ret;
}

static int check_strategy_pthread(struct cpuaffinity_enum *
				  (*strategy)(hwloc_topology_t,
					      const hwloc_obj_t),
				  int prebind)	
{
	int i, err = 0, num_threads;
	intptr_t ret;
	pthread_t tid;
	hwloc_obj_t leaf = topology_leaf(system_topology);
	struct pthread_arg parg = {
		.prebind = prebind,
		.target = NULL,
	};
	struct cpuaffinity_enum * it;
	
	it = strategy(system_topology, leaf);
	if(it == NULL)
		return 0;
	
	num_threads = hwloc_get_nbobjs_by_type(system_topology,
					       leaf->type);
	for(i=0; i<num_threads; i++){
		parg.target = cpuaffinity_enum_get(it, i);
	        assert(pthread_create(&tid, NULL, pthread_check, &parg) == 0);
		assert(pthread_join(tid, (void*)(&ret)) == 0);
		if(!ret) err++;
	}

	cpuaffinity_enum_free(it);
        return err == 0;
}
#endif

/***************************************************************************/
/*                    Check sequential, parallel, fork                     */
/***************************************************************************/

static void test_attach_parallel(int (*check_fn)(struct cpuaffinity_enum *
						 (*)(hwloc_topology_t,
						     const hwloc_obj_t),
						 int),
				 struct cpuaffinity_enum *
				 (*strategy)(hwloc_topology_t,
					     const hwloc_obj_t))
{
	struct cpuaffinity_enum * it;
	it = strategy(system_topology, topology_leaf(system_topology));
	assert(it != NULL);
	
	pid_t pid = fork();
	assert(pid >= 0);
	
	/* Tracee */
	if(pid == 0) {
		// Stop child itself, it will be resumed by ptrace or
		// killed if ptrace fails.
		kill(getpid(), SIGSTOP);
		// On resume do check. Return 0 if check succeeded.
		int status = check_fn(strategy, 0);
		exit(!status);
	}
	/* Tracer code */
	else if(pid > 0){
		int out;
		// Attach and continue execution.
	        out = cpuaffinity_attach(it, pid, 1);
		if(out < 0){
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			assert(0);
		}
		assert(out == 0);
	}	
}

static void test_strategy_parallel(int (*check_fn)(struct cpuaffinity_enum *
						   (*)(hwloc_topology_t,
						       const hwloc_obj_t),
						   int),
				   struct cpuaffinity_enum *
				   (*strategy)(hwloc_topology_t,
					       const hwloc_obj_t))
{
	struct cpuaffinity_enum * it;
	it = strategy(system_topology, topology_leaf(system_topology));
	assert(it != NULL);
	assert(check_fn(strategy, 1));
}
#endif //HWLOC_HAVE_SYS_GETTID

static void test_strategy_sequential(struct cpuaffinity_enum *
				     (*strategy)(hwloc_topology_t,
						 const hwloc_obj_t))
{
	struct cpuaffinity_enum *e;
	pid_t pid = getpid();
	hwloc_obj_t obj;
	size_t i;
	
	e = strategy(system_topology, topology_leaf(system_topology));
	assert(e != NULL);

	for(i=0; i<cpuaffinity_enum_size(e); i++){
		obj = cpuaffinity_bind_thread(e, pid);
		assert(cpuaffinity_check(system_topology, obj, pid));
	}
	
	cpuaffinity_enum_free(e);
}

/***************************************************************************/
/*                                  Run Tests                              */
/***************************************************************************/

int main(void)
{
	hwloc_topology_t topology;
	system_topology = hwloc_test_topology_load(NULL);	
	assert(system_topology != NULL);

	const struct hwloc_topology_support * support =
		hwloc_topology_get_support(system_topology);

	// Test policies are outputing expected result.
	
	test_policies(system_topology);
	DIR* xml_dir = opendir(TBIND_XML_DIR);
	if(xml_dir == NULL){
		perror("opendir");
		return 1;
	}

	char fname[512];
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
		test_policies(topology);
		hwloc_topology_destroy(topology);
	}
	
	closedir(xml_dir);


	// Test binding is done as expected.
	
	if(!support->cpubind ||
	   !support->cpubind->set_thread_cpubind ||
	   !support->cpubind->get_thread_cpubind)
		return 0;

	test_strategy_sequential(cpuaffinity_round_robin);
	test_strategy_sequential(cpuaffinity_scatter);

#ifdef HWLOC_HAVE_SYS_GETTID
#ifdef HWLOC_HAVE_PTRACE	
#ifdef _OPENMP
	test_strategy_parallel(check_strategy_openmp, cpuaffinity_round_robin);
	test_strategy_parallel(check_strategy_openmp, cpuaffinity_scatter);
	// OpenMP doesn't like to fork and hangs..
	/* test_attach_parallel(check_strategy_openmp, cpuaffinity_round_robin); */
	// OpenMP doesn't like to fork and hangs..	
	/* test_attach_parallel(check_strategy_openmp, cpuaffinity_scatter); */
#endif // _OPENMP
#ifdef HAVE_PTHREAD
	test_strategy_parallel(check_strategy_pthread, cpuaffinity_round_robin);
	test_strategy_parallel(check_strategy_pthread, cpuaffinity_scatter);
	test_attach_parallel(check_strategy_pthread, cpuaffinity_round_robin);
	test_attach_parallel(check_strategy_pthread, cpuaffinity_scatter);
#endif // HAVE_PTHREAD
#endif // HWLOC_HAVE_PTRACE
#endif // HWLOC_HAVE_SYS_GETTID
	
	hwloc_topology_destroy(system_topology);
	return 0;
}

