/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2009-2011 Université Bordeaux
 * Copyright © 2025 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_SOLARIS_SYS_MMAN_H
#define HWLOC_PORT_SOLARIS_SYS_MMAN_H

#define MADV_ACCESS_DEFAULT 6
#define MADV_ACCESS_LWP     7
#define MADV_ACCESS_MANY    8

extern int madvise(caddr_t addr, size_t len, int advice);

extern int getpagesizes(size_t pagesize[], int nelem);

#endif /* HWLOC_PORT_SOLARIS_SYS_MMAN_H */
