/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

void
look_windows(struct topo_topology *topology)
{
#ifdef WIN_SYS
#  warning: TODO: use GetLogicalProcessorInformation, GetNumaHighestNodeNumber, and GetNumaNodeProcessorMask
#endif
}
