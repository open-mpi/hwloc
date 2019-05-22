/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include "config.h"
#include "utils.h"
#include "tleaf.h"
#include "bind.h"
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>

hwloc_topology_t topology;

/********************************************************************************/
/*                         Check inside process binding                         */
/********************************************************************************/

void * bind_thread(void * arg)
{
	pid_t pid = getpid();
	struct cpuaffinity_enum *e = arg;
	hwloc_obj_t obj = cpuaffinity_bind_thread(e, pid);
	return (void*) cpuaffinity_check(topology, obj, pid);
}

#ifdef _OPENMP
#include <omp.h>

void test_openmp()
{
	struct cpuaffinity_enum *e;
	unsigned num_threads;
	int err = 0;
	
	e = cpuaffinity_scatter(topology, HWLOC_OBJ_CORE);
	
	num_threads = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
#pragma omp parallel num_threads(num_threads) shared(err)
	if(!bind_thread(e)) {err++;}
	assert(err == 0);
	cpuaffinity_enum_free(e);
}
#endif //_OPENMP

#ifdef HAVE_PTHREAD
#include <pthread.h>

void test_pthreads()
{
	struct cpuaffinity_enum *e;
	unsigned i, num_threads;
	pthread_t *tids;
	void *ret;
	int err = 0;
	
	num_threads = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	e = cpuaffinity_scatter(topology, HWLOC_OBJ_CORE);
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

/********************************************************************************/
/*                          Check attaching to a process                        */
/********************************************************************************/

#ifdef HAVE_PTHREAD
struct thread_arg{
	struct cpuaffinity_enum *e;
	int tid;
	pthread_t id;
};

void * check_pthread(void * arg)
{
	struct thread_arg *targ = arg;
	hwloc_obj_t obj = cpuaffinity_enum_get(targ->e, targ->tid);
	return (void*) cpuaffinity_check(topology, obj, getpid());
}

void run_parallel_pthread_test(struct cpuaffinity_enum *e, const unsigned num_threads)
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
void run_parallel_openmp_test(struct cpuaffinity_enum *e, const unsigned num_threads)
{
	int err = 0;	
#pragma omp parallel num_threads(num_threads) shared(err)	
	{
		int tid = omp_get_thread_num();
		fprintf(stderr, "In child %d\n", tid);			
		hwloc_obj_t obj = cpuaffinity_enum_get(e, tid);
		if(!cpuaffinity_check(topology, obj, getpid())) err++;
	}
        assert(err == 0);
}
#endif //_OPENMP

void check_attach(void(* fn)(struct cpuaffinity_enum *, const unsigned))
{
	pid_t pid;
	struct cpuaffinity_enum *e;
	unsigned num_threads;

	num_threads = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
	e = cpuaffinity_round_robin(topology, HWLOC_OBJ_CORE);
	cpuaffinity_attach(e, getpid());
	fn(e, num_threads);
}

/********************************************************************************/
/*                                Check Sequential                              */
/********************************************************************************/

void test_self()
{
	struct cpuaffinity_enum *e;
	pid_t pid = getpid();
	hwloc_obj_t obj;
	size_t i;
	
	e = cpuaffinity_round_robin(topology, HWLOC_OBJ_PU);
	assert(e != NULL);

	for(i=0; i<cpuaffinity_enum_size(e); i++){
		obj = cpuaffinity_bind_thread(e, pid);
		assert(cpuaffinity_check(topology, obj, pid));
	}
	
	cpuaffinity_enum_free(e);
}

/********************************************************************************/
/*                                 Main Routine                                 */
/********************************************************************************/

int main()
{
	topology = hwloc_affinity_topology_load(NULL);
	assert(topology != NULL);

	test_self(topology);
#ifdef _OPENMP
	test_openmp(bind_thread);
        check_attach(run_parallel_openmp_test); //Hangs because threads after fork
#endif
#ifdef HAVE_PTHREAD
	test_pthreads(bind_thread);
	check_attach(run_parallel_pthread_test); //Hangs because threads after fork
#endif
	hwloc_topology_destroy(topology);
	return 0;
}
