/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_SYS_PROCESSOR_H
#define LIBTOPOLOGY_SYS_PROCESSOR_H

/* Solaris */
#include <sys/procset.h>
typedef int processorid_t;
#define PBIND_NONE -1

extern int processor_bind(idtype_t idtype, id_t id, processorid_t processorid, processorid_t *obind);

/* AIX */
typedef short cpu_t;
#define BINDPROCESS 1
#define BINDTHREAD 2
#define PROCESSOR_CLASS_ANY ((cpu_t)(-1))
extern int bindprocessor(int What, int Who, cpu_t Where); 

#endif /* LIBTOPOLOGY_SYS_PROCESSOR_H */
