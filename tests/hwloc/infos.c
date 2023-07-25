/*
 * Copyright © 2011-2023 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "hwloc.h"

#include <assert.h>

/* check obj infos */

#define NAME1 "foobar"
#define VALUE1 "myvalue"
#define NAME2 "foobaz"
#define VALUE2 "myothervalue"

int main(void)
{
  hwloc_topology_t topology;
  hwloc_obj_t obj;
  int err;

  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);

  obj = hwloc_get_root_obj(topology);

  /* basics */
  if (hwloc_obj_get_info_by_name(obj, NAME1)
      || hwloc_obj_get_info_by_name(obj, NAME2))
    return 0;

  hwloc_obj_add_info(obj, NAME1, VALUE1);
  assert(!hwloc_obj_get_info_by_name(obj, NAME2));
  hwloc_obj_add_info(obj, NAME2, VALUE2);

  if (strcmp(hwloc_obj_get_info_by_name(obj, NAME1), VALUE1))
    assert(0);
  if (strcmp(hwloc_obj_get_info_by_name(obj, NAME2), VALUE2))
    assert(0);

  /* removes */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, VALUE1, NULL); /* no match */
  assert(!err);
  /* cannot check obj->infos.count since it contained normal info attrs after load() */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, NAME1, VALUE2); /* no match */
  assert(!err);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, NULL, NAME2); /* no match */
  assert(!err);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, NULL, NULL); /* remove all */
  assert(!err);
  assert(obj->infos.count == 0);

  /* invalid add */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, NULL, "");
  assert(err == -1);
  /* 9 interleaved duplicates */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin1", "foo1");
  assert(!err);
  assert(obj->infos.count == 1);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin2", "foo1");
  assert(!err);
  assert(obj->infos.count == 2);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin3", "foo1");
  assert(!err);
  assert(obj->infos.count == 3);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin1", "foo2");
  assert(!err);
  assert(obj->infos.count == 4);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin2", "foo2");
  assert(!err);
  assert(obj->infos.count == 5);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin3", "foo2");
  assert(!err);
  assert(obj->infos.count == 6);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin1", "foo3");
  assert(!err);
  assert(obj->infos.count == 7);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin2", "foo3");
  assert(!err);
  assert(obj->infos.count == 8);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD, "coin3", "foo3");
  assert(!err);
  assert(obj->infos.count == 9);

  /* invalid replace */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REPLACE, "", NULL);
  assert(err == -1);
  /* replace the third set of duplicates */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REPLACE, "coin3", "foo4");
  assert(!err);
  assert(obj->infos.count == 7);
  assert(!strcmp(obj->infos.array[2].name, "coin3"));
  assert(!strcmp(obj->infos.array[2].value, "foo4"));
  /* remove second set of duplicates */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, "coin2", NULL);
  assert(!err);
  assert(obj->infos.count == 4);
  /* remove second instance of first duplicates */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REMOVE, "coin1", "foo2");
  assert(!err);
  assert(obj->infos.count == 3);
  /* replace reminder of the first set of duplicates */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_REPLACE, "coin1", "foo5");
  assert(!err);
  assert(obj->infos.count == 2);
  assert(!strcmp(obj->infos.array[0].name, "coin1"));
  assert(!strcmp(obj->infos.array[0].value, "foo5"));
  /* check the other remaining entry */
  assert(!strcmp(obj->infos.array[1].name, "coin3"));
  assert(!strcmp(obj->infos.array[1].value, "foo4"));

  /* check add_unique */
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD_UNIQUE, "coin1", "foo5");
  assert(!err);
  assert(obj->infos.count == 2);
  err = hwloc_modify_infos(&obj->infos, HWLOC_MODIFY_INFOS_OP_ADD_UNIQUE, "coin1", "foo4");
  assert(!err);
  assert(obj->infos.count == 3);
  assert(!strcmp(obj->infos.array[2].name, "coin1"));
  assert(!strcmp(obj->infos.array[2].value, "foo4"));

  hwloc_topology_destroy(topology);

  return 0;
}
