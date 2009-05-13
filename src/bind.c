/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>
#include <libtopology.h>

#ifdef LINUX_SYS
#include <sched.h>

static int
topo_linux_set_cpubind(topo_cpuset_t *topo_set)
{
  /* FIXME: backport all old sched_setaffinity support from marcel_sysdep.c */
  cpu_set_t linux_set;
  unsigned cpu;

  CPU_ZERO(&linux_set);
  topo_cpuset_foreach_begin(cpu, topo_set)
    CPU_SET(cpu, &linux_set);
  topo_cpuset_foreach_end();

  return sched_setaffinity(0, sizeof(linux_set), &linux_set);
}
#endif /* LINUX_SYS */

int
topo_set_cpubind(topo_cpuset_t *set)
/* FIXME: add a pid parameter, which type is portable enough? */
{
#ifdef LINUX_SYS
  return topo_linux_set_cpubind(set);
#else
  return -1;
#endif
}
