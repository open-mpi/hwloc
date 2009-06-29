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

#ifndef LIBTOPOLOGY_RADSET_H
#define LIBTOPOLOGY_RADSET_H

typedef int radid_t;
typedef struct {
} radset_t;

extern int radsetcreate(radset_t *set);

extern int rademptyset(radset_t set);
extern int radaddset(radset_t set, radid_t radid);

extern int radsetdestroy(radset_t *set);


int rad_attach_pid(pid_t pid, radset_t radset, unsigned long flags);
int pthread_rad_attach(pthread_t thread, radset_t radset, unsigned long flags);
int rad_detach_pid(pid_t pid);
int pthread_rad_detach(pthread_t thread);


/* (strict) */
#define RAD_INSIST 1
int rad_bind_pid(pid_t pid, radset_t radset, unsigned long flags);
int pthread_rad_bind(pthread_t thread, radset_t radset, unsigned long flags);



#endif /* LIBTOPOLOGY_RADSET_H */
