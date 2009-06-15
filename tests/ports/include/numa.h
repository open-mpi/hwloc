/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_NUMA_H
#define LIBTOPOLOGY_NUMA_H

#include <radset.h>
typedef int cpu_cursor_t;
#define SET_CURSOR_INIT -1
typedef int cpuid_t;
#define CPU_NONE -1
typedef int cpuset_t;

int cpusetcreate(cpuset_t *set);
int cpuemptyset(cpuset_t set);
cpuid_t cpu_foreach(cpuset_t cpuset, int flags, cpu_cursor_t *cursor);

int rad_get_num(void);
int rad_get_cpus(radid_t rad, cpuset_t cpuset);

#endif /* LIBTOPOLOGY_NUMA_H */
