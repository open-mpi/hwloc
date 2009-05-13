/* Copyright 2009 INRIA, Université Bordeaux 1  */

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
