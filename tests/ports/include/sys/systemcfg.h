/*
 * Copyright © 2011 Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#ifndef HWLOC_PORT_SYS_SYSTEMCFG_H
#define HWLOC_PORT_SYS_SYSTEMCFG_H

struct {
  int dcache_size;
  int dcache_asc;
  int dcache_line;
  int icache_size;
  int icache_asc;
  int icache_line;
  int L2_cache_size;
  int L2_cache_asc;
  int cache_attrib;
} _system_configuration;

#endif /* HWLOC_PORT_SYS_SYSTEMCFG_H */
