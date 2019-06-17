/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
 * See COPYING in top-level directory.
 ****************************************************************************/

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include "private/autogen/config.h"
#if HWLOC_HAVE_SYS_GETTID
#include <sys/syscall.h>
#endif
#ifdef _OPENMP
#include<omp.h>
#endif
#if HWLOC_HAVE_PTHREAD
#include <pthread.h>
#endif
#include "hwloc-thread-bind.h"

int logical = 1;
int verbose = 1;
static hwloc_topology_t topology;
static struct cpuaffinity_enum *binding;

static inline hwloc_obj_t topology_leaf(void)
{
	int depth = hwloc_topology_get_depth(topology);
        return hwloc_get_obj_by_depth(topology, depth-1, 0);
}

static void hwloc_test_topology_load(void)
{
	hwloc_obj_t PU = NULL;
	int depth;
	
	if (hwloc_topology_init(&topology)) {
		perror("hwloc_topology_init");
		goto error;
	}
	if (hwloc_topology_load(topology) != 0) {
		perror("hwloc_topology_load");
		goto error_with_topology;
	}

	depth = hwloc_topology_get_depth(topology) - 1;
	binding = cpuaffinity_enum_alloc(topology);
	if(binding == NULL){
		goto error_with_topology;
	}
	while((PU = hwloc_get_next_obj_by_depth(topology, depth, PU)) != NULL){
		if(cpuaffinity_enum_append(binding, PU) != 0)
			goto error_with_binding;	
	}
	return;

 error_with_binding:
	cpuaffinity_enum_free(binding);
 error_with_topology:
	hwloc_topology_destroy(topology);
 error:
	exit(1);
}

static hwloc_obj_t
cpuaffinity_get_binding(const pid_t tid)
{
	int depth = hwloc_topology_get_depth(topology);
	hwloc_bitmap_t checkset = hwloc_bitmap_alloc();
	hwloc_obj_t bound, ret = NULL;
	
	if(hwloc_get_proc_cpubind(topology,
				  tid,
				  checkset,
				  HWLOC_CPUBIND_THREAD) == -1){
		perror("get_cpubind");
		goto check_cpubind_exit;
	}

        bound = hwloc_get_obj_by_depth(topology, depth-1, 0);
	while(bound != NULL &&
	      !hwloc_bitmap_isincluded(bound->cpuset, checkset)){
		bound = bound->next_cousin;
	}
	while(bound != NULL && bound->parent != NULL &&
	      hwloc_bitmap_isincluded(bound->parent->cpuset, checkset)){
		bound = bound->parent;
	}
	ret = bound;
	
 check_cpubind_exit:
	hwloc_bitmap_free(checkset);
	return ret;
}

static int cpuaffinity_check(const hwloc_obj_t target, const pid_t tid)
{
	int ret = 0;
	hwloc_bitmap_t checkset = hwloc_bitmap_alloc();

	hwloc_obj_t bound = cpuaffinity_get_binding(tid);
	if(bound == NULL){
		fprintf(stderr, "Binding outside of topology\n");
		goto check_cpubind_exit;
	}
	if(!hwloc_bitmap_isequal(bound->cpuset, target->cpuset)){
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

#if HWLOC_HAVE_SYS_GETTID
#ifdef _OPENMP
static int check_strategy_openmp(int prebind)	
{
	hwloc_obj_t target;
	unsigned num_threads;
	int err = 0;
	hwloc_obj_t leaf = topology_leaf();
	
	num_threads = hwloc_get_nbobjs_by_type(topology, leaf->type);
	
	if(prebind){
#pragma omp parallel num_threads(num_threads) shared(err, binding) private(target)
		{
			pid_t pid = syscall(SYS_gettid);
			target = cpuaffinity_enum_get(binding, omp_get_thread_num());
			hwloc_set_proc_cpubind(topology,
					       pid,
					       target->cpuset,
					       HWLOC_CPUBIND_THREAD);
			err += !cpuaffinity_check(target,
						  pid);
		}
		if(err != 0)
			goto out;
	}
		
#pragma omp parallel num_threads(num_threads) shared(binding, err) private(target)
	{
		pid_t pid = syscall(SYS_gettid);
		target = cpuaffinity_enum_get(binding, omp_get_thread_num());
		err += !cpuaffinity_check(target, pid);
	}

 out:
        return err == 0;
}
#endif

#if HWLOC_HAVE_PTHREAD

struct pthread_arg {
	int prebind;
	hwloc_obj_t target;
};

static void* pthread_check(void* arg)
{
	int ret = 0;
	int tid = syscall(SYS_gettid);
	struct pthread_arg *parg = arg;
	if(parg->prebind)
		hwloc_set_proc_cpubind(topology,
				       tid,
				       parg->target->cpuset,
				       HWLOC_CPUBIND_THREAD);
	// Return 1 if ok
	ret = cpuaffinity_check(parg->target, tid);
	return (void*)(intptr_t)ret;
}

static int check_strategy_pthread(int prebind)	
{
	int i, err = 0, num_threads;
	intptr_t ret = 0;
        hwloc_thread_t tid;
	hwloc_obj_t leaf = topology_leaf();
	struct pthread_arg parg = {
		.prebind = prebind,
		.target = NULL,
	};
	
	num_threads = hwloc_get_nbobjs_by_type(topology,
					       leaf->type);
	for(i=0; i<num_threads; i++){
		parg.target = cpuaffinity_enum_get(binding, i);
	        assert(pthread_create(&tid, NULL, pthread_check, &parg) == 0);
		assert(pthread_join(tid, (void**)(&ret)) == 0);
		// If ret is 0, then check failed. 
		if(!ret) err++;
	}

	// if err value is the number of failed check.
        return err == 0;
}
#endif

/***************************************************************************/
/*                    Check sequential, parallel, fork                     */
/***************************************************************************/

static void test_attach_parallel(int (*check_fn)(int))
{
	pid_t pid = fork();
	assert(pid >= 0);
	
	/* Tracee */
	if(pid == 0) {
		// Stop child itself, it will be resumed by ptrace or
		// killed if ptrace fails.
		kill(getpid(), SIGSTOP);
		// On resume do check. Return 0 if check succeeded.
		int status = check_fn(0);
		exit(!status);
	}
	/* Tracer code */
	else if(pid > 0){
		int out;
		// Attach and continue execution.
	        out = cpuaffinity_attach(pid, binding, 1, 1);
		if(out < 0){
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			assert(0);
		}
		assert(out == 0);
	}	
}

static void test_parallel(int (*check_fn)(int))
{
	// Check function return 1 if everything went as exepected
	assert(check_fn(1));
}
#endif //HWLOC_HAVE_SYS_GETTID

static void test_sequential(void)
{
	pid_t pid = getpid();
	hwloc_obj_t obj;
	size_t i;
	
	for(i=0; i<cpuaffinity_enum_size(binding); i++){
		obj = cpuaffinity_bind_thread(binding, pid);
		assert(cpuaffinity_check(obj, pid));
	}
}

/***************************************************************************/
/*                                  Run Tests                              */
/***************************************************************************/

int main(void)
{
        hwloc_test_topology_load();	

	const struct hwloc_topology_support * support =
		hwloc_topology_get_support(topology);

	// Check features are actually supported.	
	if(!support->cpubind ||
	   !support->cpubind->set_thread_cpubind ||
	   !support->cpubind->get_thread_cpubind)
		return 0;

	test_sequential();

	// Test binding is done as expected.
#if HWLOC_HAVE_SYS_GETTID
#if HWLOC_HAVE_PTRACE	
#ifdef _OPENMP
	test_parallel(check_strategy_openmp);
	//test_attach_parallel(check_strategy_openmp);
	// OpenMP doesn't like to fork and hangs..
	/* test_attach_parallel(check_strategy_openmp, cpuaffinity_round_robin); */
	// OpenMP doesn't like to fork and hangs..	
	/* test_attach_parallel(check_strategy_openmp, cpuaffinity_scatter); */
#endif // _OPENMP
#if HWLOC_HAVE_PTHREAD
	test_parallel(check_strategy_pthread);
	//test_attach_parallel(check_strategy_pthread);
#endif // HWLOC_HAVE_PTHREAD
#endif // HWLOC_HAVE_PTRACE
#endif // HWLOC_HAVE_SYS_GETTID
	
	hwloc_topology_destroy(topology);
	return 0;
}

