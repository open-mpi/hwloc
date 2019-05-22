/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include <stdlib.h>
#include <sys/ptrace.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "affinity.h"
#include "enum.h"
#include "bind.h"

int cpuaffinity_check(const hwloc_topology_t topology, const hwloc_obj_t target, const pid_t tid)
{
	int ret = 0;    
	hwloc_bitmap_t checkset = hwloc_bitmap_alloc();

	if(hwloc_get_proc_cpubind(topology, tid, checkset, HWLOC_CPUBIND_THREAD) == -1){
		perror("get_cpubind");
		goto check_cpubind_exit;
	}

	hwloc_obj_t bound = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);

	while(bound != NULL && !hwloc_bitmap_isincluded(bound->cpuset, target->cpuset)){
		if(bound->next_cousin == NULL){
			bound = hwloc_get_obj_by_depth(topology, bound->depth-1, 0);
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

hwloc_obj_t cpuaffinity_bind_thread(struct cpuaffinity_enum * objs, const pid_t tid){
	hwloc_obj_t obj = cpuaffinity_enum_next(objs);

	if(hwloc_set_proc_cpubind(objs->topology, tid, obj->cpuset, HWLOC_CPUBIND_THREAD) == -1){
		perror("hwloc_set_cpubind");
		return NULL;
	}
	return obj;
}

int cpuaffinity_attach(struct cpuaffinity_enum * objs, const pid_t pid)
{
	/* attach and set options to trace threads creation and process exit */
	if(ptrace(PTRACE_SEIZE, pid, NULL, (void*)(PTRACE_O_TRACECLONE|PTRACE_O_TRACEFORK)) == -1){
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
				/* Get ptrace event that triggered the stop */
				int event = status >> 8;
				unsigned long eventmsg;
	  
				if(ptrace(PTRACE_GETEVENTMSG, child, NULL, (void*)(&eventmsg)) == -1){
					perror("PTRACE_GETEVENTMSG");
					continue;
				}
				if(event == (SIGTRAP|(PTRACE_EVENT_FORK<<8))  ||
				   event == (SIGTRAP|(PTRACE_EVENT_VFORK<<8)) ||
				   event == (SIGTRAP|(PTRACE_EVENT_CLONE<<8))){
					cpuaffinity_bind_thread(objs, eventmsg);
				}
			}
			/* Resume stopped child */
			if(ptrace(PTRACE_CONT, child, NULL, NULL) == -1) { perror("PTRACE_CONT(interrupt)"); }	  	
		}
	} while(1);
	return 0;
}

pid_t cpuaffinity_exec(struct cpuaffinity_enum * objs, char** argv)
{
	pid_t pid = fork();  
	/* start program to trace */
	if(pid == 0) { if(execvp(argv[0], argv) == -1){ perror("execvp");  return -1; } return 0; }
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
