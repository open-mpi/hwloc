/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2012 Inria.  All rights reserved.
 * Copyright © 2009-2012 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#include <private/private.h>

int
hwloc_look_noos(struct hwloc_topology *topology)
{
  hwloc_alloc_obj_cpusets(topology->levels[0][0]);
  hwloc_setup_pu_level(topology, hwloc_fallback_nbprocessors(topology));
  if (topology->is_thissystem)
    hwloc_add_uname_info(topology);
  return 0;
}
