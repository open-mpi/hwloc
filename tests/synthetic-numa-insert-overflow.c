/*
 * Reproducer for a heap buffer overflow in the synthetic topology parser.
 *
 * Root cause (hwloc/topology-synthetic.c, "enforce a NUMA level" block):
 *   when an all-explicit, non-NUMA synthetic description reaches the maximum
 *   number of levels (HWLOC_SYNTHETIC_MAX_DEPTH-1 == 127), the memmove that
 *   shifts levels up to make room for an inserted NUMA level moved "count"
 *   elements instead of "count-1", writing one struct past the end of the
 *   fixed-size data->level[HWLOC_SYNTHETIC_MAX_DEPTH] array (which is the last
 *   member of the heap-allocated backend data).
 *
 * The description below has 125 Group levels + 1 PU level (= 126 parsed levels,
 * count reaches 127), contains no NUMA level, and uses only explicit types so
 * the auto-fill path that would otherwise insert a NUMA node is skipped. This
 * makes the buggy memmove run with count == 127 and write data->level[128].
 *
 * Build against an ASan-instrumented hwloc, e.g.:
 *   ./configure CC=clang CFLAGS="-fsanitize=address -g -O1"
 *   make
 *   clang -fsanitize=address -g tests/synthetic-numa-insert-overflow.c \
 *         -Iinclude hwloc/.libs/libhwloc.a -o repro   # (link flags vary)
 *   ./repro
 *
 * Before the fix: AddressSanitizer reports a heap-buffer-overflow WRITE inside
 * hwloc_backend_synthetic_init (the memmove). After the fix: clean exit.
 */

#include "hwloc.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
  char desc[1024];
  hwloc_topology_t topology;
  size_t off = 0;
  int i, err;

  /* 125 explicit Group levels, then the mandatory PU level. */
  for (i = 0; i < 125; i++)
    off += (size_t) snprintf(desc + off, sizeof(desc) - off, "Group:1 ");
  snprintf(desc + off, sizeof(desc) - off, "PU:1");

  if (hwloc_topology_init(&topology) < 0) {
    fprintf(stderr, "topology_init failed\n");
    return 2;
  }

  err = hwloc_topology_set_synthetic(topology, desc);
  if (err < 0) {
    fprintf(stderr, "set_synthetic rejected the string (unexpected)\n");
    hwloc_topology_destroy(topology);
    return 2;
  }

  /* The parse (and the buggy memmove) happens during load(). */
  err = hwloc_topology_load(topology);
  printf("load returned %d (no overflow detected)\n", err);

  hwloc_topology_destroy(topology);
  return 0;
}
