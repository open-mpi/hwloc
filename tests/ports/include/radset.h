/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_RADSET_H
#define LIBTOPOLOGY_RADSET_H

typedef int radid_t;
typedef struct {
} radset_t;

extern int radsetcreate(radset_t *set);

extern int rademptyset(radset_t set);
extern int radaddset(radset_t set, radid_t radid);

extern int radsetdestroy(radset_t *set);


int pthread_rad_attach(pthread_t thread, radset_t radset, unsigned long flags);

/* (strict) */
#define RAD_INSIST 1
int pthread_rad_bind(pthread_t thread, radset_t radset, unsigned long flags);


#endif /* LIBTOPOLOGY_RADSET_H */
