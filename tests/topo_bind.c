/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <errno.h>

#include <hwloc.h>
#include <private/config.h>

/* check the binding functions */
topo_topology_t topology;

static void result(const char *msg, int err)
{
  if (err)
    printf("%-30s: FAILED (%d, %s)\n", msg, errno, strerror(errno));
  else
    printf("%-30s: OK\n", msg);
}

static void test(topo_cpuset_t *cpuset, int flags)
{
  result("Bind singlethreaded process", topo_set_cpubind(topology, cpuset, flags));
  result("Bind this thread", topo_set_cpubind(topology, cpuset, flags | TOPO_CPUBIND_THREAD));
  result("Bind this whole process", topo_set_cpubind(topology, cpuset, flags | TOPO_CPUBIND_PROCESS));

#ifdef WIN_SYS
  result("Bind process", topo_set_proc_cpubind(topology, GetCurrentProcess(), cpuset, flags | TOPO_CPUBIND_PROCESS));
  result("Bind thread", topo_set_thread_cpubind(topology, GetCurrentThread(), cpuset, flags | TOPO_CPUBIND_THREAD));
#else /* !WIN_SYS */
  result("Bind process", topo_set_proc_cpubind(topology, getpid(), cpuset, flags | TOPO_CPUBIND_PROCESS));
#ifdef hwloc_thread_t
  result("Bind thread", topo_set_thread_cpubind(topology, pthread_self(), cpuset, flags | TOPO_CPUBIND_THREAD));
#endif
#endif /* !WIN_SYS */
  printf("\n");
}

int main(void)
{
  topo_cpuset_t set;
  topo_obj_t obj;
  char s[TOPO_CPUSET_STRING_LENGTH + 1];

  topo_topology_init(&topology);
  topo_topology_load(topology);

  obj = topo_get_system_obj(topology);
  set = obj->cpuset;

  while (topo_cpuset_isequal(&obj->cpuset, &set)) {
    if (!obj->arity)
      break;
    obj = obj->children[0];
  }

  topo_cpuset_snprintf(s, sizeof(s), &set);
  printf("system set is %s\n", s);

  test(&set, 0);

  printf("now strict\n");
  test(&set, TOPO_CPUBIND_STRICT);

  set = obj->cpuset;
  topo_cpuset_snprintf(s, sizeof(s), &set);
  printf("obj set is %s\n", s);

  test(&set, 0);

  printf("now strict\n");
  test(&set, TOPO_CPUBIND_STRICT);

  topo_cpuset_singlify(&set);
  topo_cpuset_snprintf(s, sizeof(s), &set);
  printf("singlified to %s\n", s);

  test(&set, 0);

  printf("now strict\n");
  test(&set, TOPO_CPUBIND_STRICT);

  topo_topology_destroy(topology);
  return 0;
}
