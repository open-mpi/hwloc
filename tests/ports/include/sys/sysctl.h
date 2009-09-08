/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#ifndef LIBTOPOLOGY_SYS_SYSCTL_H
#define LIBTOPOLOGY_SYS_SYSCTL_H

extern int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

#endif /* LIBTOPOLOGY_SYS_SYSCTL_H */
