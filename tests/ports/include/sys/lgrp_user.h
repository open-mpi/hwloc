/* Copyright 2009 INRIA, Université Bordeaux 1  */

#ifndef LIBTOPOLOGY_SYS_LGRP_USER_H
#define LIBTOPOLOGY_SYS_LGRP_USER_H

typedef int lgrp_cookie_t;
#define LGRP_COOKIE_NONE 0
typedef int lgrp_id_t;
typedef int processorid_t;

typedef enum lgrp_content {
	LGRP_CONTENT_ALL,
	LGRP_CONTENT_HIERARCHY = LGRP_CONTENT_ALL,
	LGRP_CONTENT_DIRECT
} lgrp_content_t;

typedef enum lgrp_view {
	LGRP_VIEW_CALLER,
	LGRP_VIEW_OS,
} lgrp_view_t;

lgrp_cookie_t lgrp_init(lgrp_view_t view);

lgrp_id_t lgrp_root(lgrp_cookie_t cookie);
int lgrp_cpus(lgrp_cookie_t cookie, lgrp_id_t lgrp, processorid_t *cpuids, unsigned int count, int content);
int lgrp_children(lgrp_cookie_t cookie, lgrp_id_t parent, lgrp_id_t *lgrp_array, unsigned int lgrp_array_size);

int lgrp_fini();

#endif /* LIBTOPOLOGY_SYS_LGRP_USER_H */
