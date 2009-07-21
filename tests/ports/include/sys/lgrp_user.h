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

#ifndef LIBTOPOLOGY_SYS_LGRP_USER_H
#define LIBTOPOLOGY_SYS_LGRP_USER_H

#include <sys/processor.h>

typedef int lgrp_cookie_t;
#define LGRP_COOKIE_NONE 0
typedef int lgrp_id_t;
typedef long long lgrp_mem_size_t;

typedef enum lgrp_content {
	LGRP_CONTENT_ALL,
	LGRP_CONTENT_HIERARCHY = LGRP_CONTENT_ALL,
	LGRP_CONTENT_DIRECT
} lgrp_content_t;

typedef enum lgrp_view {
	LGRP_VIEW_CALLER,
	LGRP_VIEW_OS,
} lgrp_view_t;

typedef enum lgrp_mem_size_flag {
	LGRP_MEM_SZ_FREE,
	LGRP_MEM_SZ_INSTALLED,
} lgrp_mem_size_flag_t;

typedef enum lgrp_lat_between {
	LGRP_LAT_CPU_TO_MEM
} lgrp_lat_between_t;

lgrp_cookie_t lgrp_init(lgrp_view_t view);

int lgrp_nlgrps(lgrp_cookie_t cookie);
lgrp_id_t lgrp_root(lgrp_cookie_t cookie);
int lgrp_cpus(lgrp_cookie_t cookie, lgrp_id_t lgrp, processorid_t *cpuids, unsigned int count, int content);
int lgrp_children(lgrp_cookie_t cookie, lgrp_id_t parent, lgrp_id_t *lgrp_array, unsigned int lgrp_array_size);
lgrp_mem_size_t lgrp_mem_size(lgrp_cookie_t cookie, lgrp_id_t lgrp, lgrp_mem_size_flag_t type, lgrp_content_t content);
int lgrp_latency_cookie(lgrp_cookie_t cookie, lgrp_id_t from, lgrp_id_t to, lgrp_lat_between_t between);

int lgrp_fini();

#endif /* LIBTOPOLOGY_SYS_LGRP_USER_H */
