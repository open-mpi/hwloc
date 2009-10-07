/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <hwloc.h>
#include <private/config.h>

/* check the binding functions */
hwloc_topology_t topology;

static void result(const char *msg, int err)
{
  if (err)
    printf("%-30s: FAILED (%d, %s)\n", msg, errno, strerror(errno));
  else
    printf("%-30s: OK\n", msg);
}

static void test(hwloc_cpuset_t cpuset, int flags)
{
  result("Bind singlethreaded process", hwloc_set_cpubind(topology, cpuset, flags));
  result("Bind this thread", hwloc_set_cpubind(topology, cpuset, flags | HWLOC_CPUBIND_THREAD));
  result("Bind this whole process", hwloc_set_cpubind(topology, cpuset, flags | HWLOC_CPUBIND_PROCESS));

#ifdef WIN_SYS
  result("Bind process", hwloc_set_proc_cpubind(topology, GetCurrentProcess(), cpuset, flags | HWLOC_CPUBIND_PROCESS));
  result("Bind thread", hwloc_set_thread_cpubind(topology, GetCurrentThread(), cpuset, flags | HWLOC_CPUBIND_THREAD));
#else /* !WIN_SYS */
  result("Bind process", hwloc_set_proc_cpubind(topology, getpid(), cpuset, flags | HWLOC_CPUBIND_PROCESS));
#ifdef hwloc_thread_t
  result("Bind thread", hwloc_set_thread_cpubind(topology, pthread_self(), cpuset, flags | HWLOC_CPUBIND_THREAD));
#endif
#endif /* !WIN_SYS */
  printf("\n");
}

int main(void)
{
  hwloc_cpuset_t set;
  hwloc_obj_t obj;
  char *str = NULL;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  obj = hwloc_get_system_obj(topology);
  set = hwloc_cpuset_dup(obj->cpuset);

  while (hwloc_cpuset_isequal(obj->cpuset, set)) {
    if (!obj->arity)
      break;
    obj = obj->children[0];
  }

  hwloc_cpuset_asprintf(&str, set);
  printf("system set is %s\n", str);
  free(str);

  test(set, 0);

  printf("now strict\n");
  test(set, HWLOC_CPUBIND_STRICT);

  hwloc_cpuset_free(set);
  set = hwloc_cpuset_dup(obj->cpuset);
  hwloc_cpuset_asprintf(&str, set);
  printf("obj set is %s\n", str);
  free(str);

  test(set, 0);

  printf("now strict\n");
  test(set, HWLOC_CPUBIND_STRICT);

  hwloc_cpuset_singlify(set);
  hwloc_cpuset_asprintf(&str, set);
  printf("singlified to %s\n", str);
  free(str);

  test(set, 0);

  printf("now strict\n");
  test(set, HWLOC_CPUBIND_STRICT);

  hwloc_cpuset_free(set);
  hwloc_topology_destroy(topology);
  return 0;
}
