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

#include <config.h>
#include <topology.h>

#ifdef LINUX_SYS
#include <sched.h>

#  ifndef CPU_SET
/* libc doesn't have support for sched_setaffinity, build system call
 * ourselves: */
#    include <linux/unistd.h>
#    ifndef __NR_sched_setaffinity
#       ifdef __i386__
#         define __NR_sched_setaffinity 241
#       elif defined(__x86_64__)
#         define __NR_sched_setaffinity 203
#       elif defined(__ia64__)
#         define __NR_sched_setaffinity 1231
#       else
#         error "don't know the syscall number for sched_setaffinity on this architecture"
#       endif
_syscall3(int, sched_setaffinity, pid_t, pid, unsigned int, lg, unsigned long *, mask);
#    endif
#  endif

static int
topo_linux_set_cpubind(const topo_cpuset_t *topo_set)
{
#ifdef CPU_SET
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  topo_cpuset_foreach_begin(cpu, topo_set)
    CPU_SET(cpu, &linux_set);
  topo_cpuset_foreach_end();

#ifdef HAVE_OLD_SCHED_SETAFFINITY
  return sched_setaffinity(0, &linux_set);
#else /* HAVE_OLD_SCHED_SETAFFINITY */
  return sched_setaffinity(0, sizeof(linux_set), &linux_set);
#endif /* HAVE_OLD_SCHED_SETAFFINITY */
#else /* CPU_SET */
  unsigned long mask = topo_cpuset_to_ulong(topo_set);

  return sched_setaffinity(0, sizeof(mask), &mask);
#endif /* CPU_SET */
}
#endif /* LINUX_SYS */

#ifdef SOLARIS_SYS
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>

static int
topo_solaris_set_cpubind(const topo_cpuset_t *topo_set)
{
  unsigned target;

  if (topo_cpuset_isfull(topo_set)) {
    if (processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL) != 0)
      return -1;
    return 0;
  }

  if (topo_cpuset_weight(topo_set) != 1)
    return -1;

  target = topo_cpuset_first(topo_set);

  if (processor_bind(P_LWPID, P_MYID,
		     (processorid_t) (target), NULL) != 0)
    return -1;

  return 0;
}
#endif /* SOLARIS_SYS */

#ifdef WIN_SYS
#include <windows.h>
static int
topo_win_set_cpubind(const topo_cpuset_t *topo_set)
{
  DWORD mask = topo_cpuset_to_ulong(topo_set);
  if (!SetThreadAffinityMask(GetCurrentThread(), mask))
    return -1;
  return 0;
}
#endif /* WIN_SYS */

#ifdef OSF_SYS
#include <radset.h>
#include <numa.h>
static int
topo_osf_set_cpubind(const topo_cpuset_t *topo_set)
{
  radset_t radset;
  unsigned cpu;

  radsetcreate(&radset);
  rademptyset(radset);
  topo_cpuset_foreach_begin(cpu, topo_set)
    radaddset(radset, cpu);
  if (pthread_rad_bind(pthread_self(), radset, RAD_INSIST))
    return -1;
  radsetdestroy(&radset);

  return 0;
}

/* TODO: process: bind_to_cpu(), bind_to_cpu_id(), rad_bind_pid(),
 * nsg_init(), nsg_attach_pid()
 * assign_pid_to_pset()
 * */
#endif /* OSF_SYS */

#ifdef AIX_SYS
#include <sys/processor.h>
#include <sys/thread.h>
static int
topo_aix_set_cpubind(const topo_cpuset_t *topo_set)
{
  unsigned target;

  if (topo_cpuset_isfull(topo_set)) {
#warning TODO unbind thread
  }

  if (topo_cpuset_weight(topo_set) != 1)
    return -1;

  target = topo_cpuset_first(topo_set);

  /* TODO: pthread/thread: pthdb_pthread_tid / pthdb_tid_pthread */

  /* TODO: NUMA: ra_attachrset / rs_setpartition */

  if (bindprocessor(BINDTHREAD, thread_self(), target))
    return -1;

  return 0;
}
#endif /* AIX_SYS */

/* TODO: GNU_SYS, FREEBSD_SYS, DARWIN_SYS, IRIX_SYS, HPUX_SYS
 * IRIX: see _DSM_MUSTRUN */

int
topo_set_cpubind(const topo_cpuset_t *set)
/* FIXME: add a pid parameter, which type is portable enough?  POSIX says that
 * pid_t shall be a signed integer type, on windows that can be a handle
 * (pointer) or an id.  */
{
#ifdef LINUX_SYS
  return topo_linux_set_cpubind(set);
#elif defined(SOLARIS_SYS)
  return topo_solaris_set_cpubind(set);
#elif defined(WIN_SYS)
  return topo_win_set_cpubind(set);
#elif defined(OSF_SYS)
  return topo_osf_set_cpubind(set);
#elif defined(AIX_SYS)
  return topo_aix_set_cpubind(set);
#else
#warning "don't know how to bind on processors on this system"
  return -1;
#endif
}

/* TODO: memory bind */
