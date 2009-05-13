/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_KSTAT_H
#define LIBTOPOLOGY_KSTAT_H

#include <stdint.h>

typedef int kid_t;

#define KSTAT_STRLEN 31

typedef struct kstat {
  char ks_module[KSTAT_STRLEN];
  int ks_instance;
  struct kstat *ks_next;
} kstat_t;

typedef struct kstat_named {
  unsigned char data_type;
  union {
    int32_t i32;
  } value;
} kstat_named_t;

typedef struct kstat_ctl {
  kstat_t *kc_chain;
} kstat_ctl_t;

#define KSTAT_DATA_INT32 1

kstat_ctl_t *kstat_open(void);
kid_t kstat_read(kstat_ctl_t *, kstat_t *, void *);
void *kstat_data_lookup(kstat_t *, char *);
int kstat_close(kstat_ctl_t *);

#endif /* LIBTOPOLOGY_KSTAT_H */
