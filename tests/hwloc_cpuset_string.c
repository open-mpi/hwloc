/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

#include <hwloc.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check hwloc_cpuset_asprintf(), hwloc_obj_cpuset_snprintf() and hwloc_cpuset_from_string() */

int main()
{
  hwloc_topology_t topology;
  unsigned depth;
  char *string = NULL;
  int stringlen, len;
  hwloc_obj_t obj;
  hwloc_cpuset_t set;

  hwloc_topology_init(&topology);
  hwloc_topology_set_synthetic(topology, "6 5 4 3 2");
  hwloc_topology_load(topology);
  depth = hwloc_topology_get_depth(topology);

  obj = hwloc_get_system_obj(topology);
  stringlen = hwloc_cpuset_asprintf(&string, obj->cpuset);
  printf("system cpuset is %s\n", string);
  set = hwloc_cpuset_from_string(string);
  assert(hwloc_cpuset_isequal(set, obj->cpuset));
  free(set);
  printf("system cpuset converted back and forth, ok\n");

  printf("truncating system cpuset to NULL buffer\n");
  len = hwloc_obj_cpuset_snprintf(NULL, 0, 1, &obj);
  assert(len == stringlen);

  printf("truncating system cpuset to 0 char (no modification)\n");
  memset(string, 'X', 1);
  string[1] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 0, 1, &obj);
  assert(len == stringlen);
  assert(string[0] == 'X');

  printf("truncating system cpuset to 1 char (empty string)\n");
  memset(string, 'X', 2);
  string[2] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 1, 1, &obj);
  printf("got %s\n", string);
  assert(len == stringlen);
  assert(string[0] == 0);
  assert(string[1] == 'X');

  printf("truncating system cpuset to 8 chars (single 32bit subset except last char)\n");
  memset(string, 'X', 9);
  string[9] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 8, 1, &obj);
  printf("got %s\n", string);
  assert(len == stringlen);
  assert(string[6] == 'f');
  assert(string[7] == 0);
  assert(string[8] == 'X');

  printf("truncating system cpuset to 9 chars (single 32bit subset)\n");
  memset(string, 'X', 10);
  string[10] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 9, 1, &obj);
  printf("got %s\n", string);
  assert(len == stringlen);
  assert(string[7] == 'f');
  assert(string[8] == 0);
  assert(string[9] == 'X');

  printf("truncating system cpuset to 19 chars (two 32bit subsets with ending comma)\n");
  memset(string, 'X', 20);
  string[20] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 19, 1, &obj);
  printf("got %s\n", string);
  assert(len == stringlen);
  assert(string[16] == 'f');
  assert(string[17] == ',');
  assert(string[18] == 0);
  assert(string[19] == 'X');

  printf("truncating system cpuset to 41 chars (truncate to four and a half 32bit subsets)\n");
  memset(string, 'X', 42);
  string[42] = 0;
  len = hwloc_obj_cpuset_snprintf(string, 41, 1, &obj);
  printf("got %s\n", string);
  assert(len == stringlen);
  assert(string[39] == 'f');
  assert(string[40] == 0);
  assert(string[41] == 'X');

  obj = hwloc_get_obj_by_depth(topology, depth-1, 0);
  hwloc_obj_cpuset_snprintf(string, stringlen+1, 1, &obj);
  printf("first cpu cpuset is %s\n", string);
  set = hwloc_cpuset_from_string(string);
  assert(hwloc_cpuset_isequal(set, obj->cpuset));
  free(set);
  printf("first cpu cpuset converted back and forth, ok\n");

  obj = hwloc_get_obj_by_depth(topology, depth-1, hwloc_get_nbobjs_by_depth(topology, depth-1) - 1);
  hwloc_obj_cpuset_snprintf(string, stringlen+1, 1, &obj);
  printf("last cpu cpuset is %s\n", string);
  set = hwloc_cpuset_from_string(string);
  assert(hwloc_cpuset_isequal(set, obj->cpuset));
  free(set);
  printf("last cpu cpuset converted back and forth, ok\n");

//  hwloc_cpuset_from_string("1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,2,4,8,10,20\n", &set);
//  char *s = hwloc_cpuset_printf_value(&set);
//  printf("%s\n", s);
//  free(s);
//  will be truncated after ",4," since it's too large

  free(string);

  hwloc_topology_destroy(topology);

  return 0;
}
