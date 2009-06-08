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

#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdlib.h>

#include <topology.h>
#include <topology/helper.h>
#include <topology/debug.h>

static int get_sysctl(const char *name, int *res)
{
  int n;
  size_t size = sizeof(n);
  if (sysctlbyname(name, &n, &size, NULL, 0))
    return -1;
  if (size != sizeof(n))
    return -1;
  *res = n;
  return 0;
}

void
topo_look_darwin(struct topo_topology *topology)
{
  int nprocs;
  int npackages;
  int i, cpu;
  struct topo_obj *obj;

  if (get_sysctl("hw.ncpu", &nprocs))
    return;

  topo_debug("%d procs\n", nprocs);

  if (!get_sysctl("hw.packages", &npackages)) {
    int cores_per_package;
    int logical_per_package;

    topo_debug("%d packages\n", npackages);

    if (get_sysctl("machdep.cpu.logical_per_package", &logical_per_package))
      /* Assume the trivia.  */
      logical_per_package = nprocs / npackages;

    topo_debug("%d threads per package\n", logical_per_package);

    assert(nprocs == npackages * logical_per_package);

    for (i = 0; i < npackages; i++) {
      obj = topo_alloc_setup_object(TOPO_OBJ_SOCKET, i);
      for (cpu = i*logical_per_package; cpu < (i+1)*logical_per_package; cpu++)
	topo_cpuset_set(&obj->cpuset, cpu);

      topo_debug("package %d has cpuset %"TOPO_PRIxCPUSET"\n",
		 i, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
      topo_add_object(topology, obj);
    }

    if (!get_sysctl("machdep.cpu.cores_per_package", &cores_per_package)) {
      topo_debug("%d cores per package\n", cores_per_package);

      assert(!(logical_per_package % cores_per_package));

      for (i = 0; i < npackages * cores_per_package; i++) {
	obj = topo_alloc_setup_object(TOPO_OBJ_CORE, i);
	for (cpu = i*(logical_per_package/cores_per_package);
	     cpu < (i+1)*(logical_per_package/cores_per_package);
	     cpu++)
	  topo_cpuset_set(&obj->cpuset, cpu);

	topo_debug("core %d has cpuset %"TOPO_PRIxCPUSET"\n",
		   i, TOPO_CPUSET_PRINTF_VALUE(&obj->cpuset));
	topo_add_object(topology, obj);
      }
    }
  }

  /* add PROC objects */
  topo_setup_proc_level(topology, nprocs, NULL);
}
