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

#ifndef LIBTOPOLOGY_SYS_RSET_H
#define LIBTOPOLOGY_SYS_RSET_H

typedef void *rsethandle_t;

#define RS_EMPTY 3
#define RS_ALL 2
#define RS_PARTITION 3

#define RS_TESTRESOURCE 11

typedef int rsinfo_t;
#define R_NUMPROCS 0
#define R_MAXSDL 3
#define R_SMPSDL 4
#define R_MCMSDL 5
#define R_MAXPROCS 6

#define R_PROCS 6

rsethandle_t rs_alloc (unsigned int flags);
int rs_numrads(rsethandle_t rset, unsigned int sdl, unsigned int flags);
int rs_getrad (rsethandle_t rset, rsethandle_t rad, unsigned int sdl, unsigned int index, unsigned int flags);
int rs_getinfo(rsethandle_t rseth, rsinfo_t info_type, unsigned int flags);
int rs_op(unsigned int command, rsethandle_t rseth1, rsethandle_t rseth2, unsigned int flags, unsigned int id);
void rs_free(rsethandle_t rseth);

#endif /* LIBTOPOLOGY_SYS_RSET_H */
