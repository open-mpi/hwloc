/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2025 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_FREEBSD_SYS_MMAN_H
#define HWLOC_PORT_FREEBSD_SYS_MMAN_H

/* use uint64_t for pagesize since this port test runs on Linux */
extern int getpagesizes(uint64_t pagesize[], int nelem);

#endif /* HWLOC_PORT_FREEBSD_SYS_MMAN_H */
