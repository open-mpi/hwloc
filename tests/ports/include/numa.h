/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

#ifndef LIBTOPOLOGY_NUMA_H
#define LIBTOPOLOGY_NUMA_H

#include <sys/types.h>
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
ssize_t rad_get_physmem(radid_t rad);

#endif /* LIBTOPOLOGY_NUMA_H */
