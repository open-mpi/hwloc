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

#include <stdio.h>
#include <errno.h>

#include <topology.h>
#include <config.h>

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
  result("Bind thread", topo_set_thread_cpubind(topology, pthread_self(), cpuset, flags | TOPO_CPUBIND_THREAD));
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
