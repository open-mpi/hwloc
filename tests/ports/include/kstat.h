/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

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
