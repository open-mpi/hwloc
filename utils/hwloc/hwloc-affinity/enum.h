/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#ifndef CPUAFFINITY_ENUM_H
#define CPUAFFINITY_ENUM_H

struct cpuaffinity_enum{
	/** The topology used to build enum **/
	hwloc_topology_t topology;
	/** Index of current hwloc_obj in enumeration **/
        unsigned current;
	/** Number of hwloc_obj in enumeration **/
	unsigned n;
	/** 
	 * Maximum number of hwloc_obj storable. 
	 * This the number of HWLOC_OBJ_PU in topology.
	 **/
	unsigned nmax;
	/** 
	 * Array of processing units.
	 * These are the pointers from
	 * an existing topology. Topology must not 
	 * be destroyed until the enum is.
	 **/
	hwloc_obj_t *obj;
};

#endif //CPUAFFINITY_ENUM_H
