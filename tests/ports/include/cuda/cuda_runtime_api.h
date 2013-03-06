/*
 * Copyright © 2013 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_CUDA_CUDA_RUNTIME_API_H
#define HWLOC_PORT_CUDA_CUDA_RUNTIME_API_H

typedef unsigned cudaError_t;

struct cudaDeviceProp {
  char * name;
  int pciBusID;
  int pciDeviceID;
};

cudaError_t cudaGetDeviceProperties(struct cudaDeviceProp *, int);
cudaError_t cudaGetDeviceCount(int *);

#endif /* HWLOC_PORT_CUDA_CUDA_RUNTIME_API_H */
