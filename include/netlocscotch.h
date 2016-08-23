/*
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#ifndef _NETLOCSCOTCH_H_
#define _NETLOCSCOTCH_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // for asprintf
#endif

#include <hwloc/autogen/config.h>

/* Includes for Scotch */
#include <stdio.h>
#include <scotch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *nodename;
    int core;
    int rank;
} netlocscotch_core_t;

/**
 * \brief Give a good mapping with Scotch from a communication matrix
 *
 * This function reads the file about available resources, found by reading the
 * environment variable NETLOC_CURRENTSLOTS. The file must be generated before
 * calling the program running this functions with: mpirun -np <nprocs>
 * netloc_mpi_find_hosts <outputfile>
 *
 * \param filname Filename of the matrix file, where the matrix is stored line
 * by line with spaces between values
 *
 * \param pnum_processes Pointer to the integer where th number of processes
 * will be written
 *
 * \param pcores Array of pnum_processes elements 
 *
 * \returns 0 on succes 
 * \returns NETLOC_ERROR on error
 */
int netlocscotch_get_mapping_from_comm_file(char *filename, int *pnum_processes,
        netlocscotch_core_t **pcores);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif // _NETLOC_H_
