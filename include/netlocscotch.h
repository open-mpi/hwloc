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

#include <scotch.h>

#ifdef __cplusplus
extern "C" {
#endif

int netlocscotch_build_current_arch(SCOTCH_Arch *subarch);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif // _NETLOC_H_
