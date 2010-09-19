/*
 * Copyright © 2010 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <stdio.h>
#include <assert.h>
#include <cuda_runtime_api.h>
#include <private/config.h>
#include <hwloc.h>
#include <hwloc/cudart.h>

/* check the CUDA Runtime API helpers */

int main(void)
{
  hwloc_topology_t topology;
  cudaError_t cerr;
  int count, i;

  cerr = cudaGetDeviceCount(&count);
  if (cerr) {
    printf("cudaGetDeviceCount failed %d\n", cerr);
    return -1;
  }
  printf("cudaGetDeviceCount found %d devices\n", count);

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  for(i=0; i<count; i++) {
    hwloc_cpuset_t set;

    set = hwloc_cpuset_alloc();
    hwloc_cudart_get_device_cpuset(topology, i, set);
    if (!set) {
      printf("failed to get cpuset for device %d\n", i);
    } else {
      char *cpuset_string = NULL;
      hwloc_cpuset_asprintf(&cpuset_string, set);
      printf("got cpuset %s for device %d\n", cpuset_string, i);
      free(cpuset_string);
    }
    hwloc_cpuset_free(set);
  }

  hwloc_topology_destroy(topology);

  return 0;
}
