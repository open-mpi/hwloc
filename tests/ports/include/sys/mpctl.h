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

#ifndef LIBTOPOLOGY_SYS_MPCTL_H
#define LIBTOPOLOGY_SYS_MPCTL_H

typedef int spu_t, ldom_t, pthread_spu_t, pthread_ldom_t;

typedef enum mpc_request {
  MPC_GETNUMSPUS,
  MPC_GETFIRSTSPU,
  MPC_GETNEXTSPU,

  MPC_GETNUMSPUS_SYS,
  MPC_GETFIRSTSPU_SYS,
  MPC_GETNEXTSPU_SYS,

  MPC_GETCURRENTSTSPU,
  MPC_SETPROCESS,
  MPC_SETPROCESS_FORCE,
  MPC_SETLWP,
  MPC_SETLWP_FORCE,
  MPC_SETLWP_UP,

  MPC_SETLDOM,
  MPC_SETLWPLDOM,

  MPC_GETNUMLDOMS,
  MPC_GETFIRSTLDOM,
  MPC_GETNEXTLDOM,
  MPC_GETNUMLDOMS_SYS,
  MPC_GETFIRSTLDOM_SYS,
  MPC_GETNEXTLDOM_SYS,

  MPC_SPUTOLDOM,
  MPC_LDOMSPUS,
  MPC_LDOMSPUS_SYS,

  MPC_GETPROCESS_BINDVALUE,
  MPC_GETLWP_BINDVALUE
} mpc_request_t;

#define MPC_SPUFLOAT 0
#define MPC_LDOMFLOAT 1
#define MPC_SELFPID 2
#define MPC_SELFLWPPID 3

extern int mpctl(mpc_request_t, spu_t, ...);

#define _SC_CCNUMA_SUPPORT 0
#define _SC_PSET_SUPPORT 1

int pthread_processor_bind_np(int request, pthread_spu_t *answer, pthread_spu_t spu, pthread_t tid);

int pthread_ldom_bind_np(pthread_ldom_t *answer, pthread_ldom_t ldom, pthread_t tid);

#define PTHREAD_SELFTID_NP 0
#define PTHREAD_LDOMFLOAT_NP 0
#define PTHREAD_SPUFLOAT_NP 0

#define PTHREAD_BIND_ADVISORY_NP 0
#define PTHREAD_BIND_FORCED_NP 1

#endif /* LIBTOPOLOGY_SYS_MPCTL_H */
