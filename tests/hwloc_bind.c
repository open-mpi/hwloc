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

static void result_set(const char *msg, int err)
{
  if (err)
    printf("%-30s: FAILED (%d, %s)\n", msg, errno, strerror(errno));
  else
    printf("%-30s: OK\n", msg);
}

static void result_get(const char *msg, hwloc_cpuset_t expected, hwloc_cpuset_t result)
{
  if (!result)
    printf("%-30s: FAILED (%d, %s)\n", msg, errno, strerror(errno));
  else if (hwloc_cpuset_isequal(expected, result))
    printf("%-30s: OK\n", msg);
  else {
    char *expected_s, *result_s;
    hwloc_cpuset_asprintf(&expected_s, expected);
    hwloc_cpuset_asprintf(&result_s, result);
    printf("%-30s: expected %s, got %s\n", msg, expected_s, result_s);
  }
}

static void test(hwloc_cpuset_t cpuset, int flags)
{
  result_set("Bind singlethreaded process", hwloc_set_cpubind(topology, cpuset, flags));
  result_get("Get  singlethreaded process", cpuset, hwloc_get_cpubind(topology, flags));
  result_set("Bind this thread", hwloc_set_cpubind(topology, cpuset, flags | HWLOC_CPUBIND_THREAD));
  result_get("Get  this thread", cpuset, hwloc_get_cpubind(topology, flags | HWLOC_CPUBIND_THREAD));
  result_set("Bind this whole process", hwloc_set_cpubind(topology, cpuset, flags | HWLOC_CPUBIND_PROCESS));
  result_get("Get  this whole process", cpuset, hwloc_get_cpubind(topology, flags | HWLOC_CPUBIND_PROCESS));

#ifdef HWLOC_WIN_SYS
  result_set("Bind process", hwloc_set_proc_cpubind(topology, GetCurrentProcess(), cpuset, flags | HWLOC_CPUBIND_PROCESS));
  result_get("Get  process", cpuset, hwloc_get_proc_cpubind(topology, GetCurrentProcess(), flags | HWLOC_CPUBIND_PROCESS));
  result_set("Bind thread", hwloc_set_thread_cpubind(topology, GetCurrentThread(), cpuset, flags | HWLOC_CPUBIND_THREAD));
  result_get("Get  thread", cpuset, hwloc_get_thread_cpubind(topology, GetCurrentThread(), flags | HWLOC_CPUBIND_THREAD));
#else /* !HWLOC_WIN_SYS */
  result_set("Bind whole process", hwloc_set_proc_cpubind(topology, getpid(), cpuset, flags | HWLOC_CPUBIND_PROCESS));
  result_get("Get  whole process", cpuset, hwloc_get_proc_cpubind(topology, getpid(), flags | HWLOC_CPUBIND_PROCESS));
  result_set("Bind process", hwloc_set_proc_cpubind(topology, getpid(), cpuset, flags));
  result_get("Get  process", cpuset, hwloc_get_proc_cpubind(topology, getpid(), flags));
#ifdef hwloc_thread_t
  result_set("Bind thread", hwloc_set_thread_cpubind(topology, pthread_self(), cpuset, flags));
  result_get("Get thread", cpuset, hwloc_get_thread_cpubind(topology, pthread_self(), flags));
#endif
#endif /* !HWLOC_WIN_SYS */
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
