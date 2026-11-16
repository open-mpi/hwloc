/*
 * SPDX-License-Identifier: BSD-3-Clause
 * See COPYING in top-level directory.
 */

#include "hwloc.h"
#include "private/private.h" /* for hwloc_snprintf() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* check that hwloc_snprintf() 0-terminates truncated output without writing
 * past the given size, and returns the length of the untruncated output.
 * many callers print such buffers with %s or strlen() them afterwards.
 * explicit checks instead of assert() so that this also works with NDEBUG.
 */

int main(void)
{
  char buf[8];
  volatile size_t size; /* not a constant to avoid truncation warnings */
  int ret;

  /* truncated string */
  memset(buf, 'x', sizeof(buf));
  size = 4;
  ret = hwloc_snprintf(buf, size, "%s", "abcdef");
  if (ret != 6 || buf[3] != '\0' || strcmp(buf, "abc") || buf[4] != 'x') {
    fprintf(stderr, "hwloc_snprintf() mishandled truncated string output (returned %d)\n", ret);
    return EXIT_FAILURE;
  }

  /* truncated integer */
  memset(buf, 'x', sizeof(buf));
  size = 4;
  ret = hwloc_snprintf(buf, size, "%d", 123456);
  if (ret != 6 || buf[3] != '\0' || strcmp(buf, "123") || buf[4] != 'x') {
    fprintf(stderr, "hwloc_snprintf() mishandled truncated integer output (returned %d)\n", ret);
    return EXIT_FAILURE;
  }

  /* output and terminating 0 exactly fit */
  memset(buf, 'x', sizeof(buf));
  size = 7;
  ret = hwloc_snprintf(buf, size, "%s", "abcdef");
  if (ret != 6 || buf[6] != '\0' || strcmp(buf, "abcdef") || buf[7] != 'x') {
    fprintf(stderr, "hwloc_snprintf() mishandled exactly fitting output (returned %d)\n", ret);
    return EXIT_FAILURE;
  }

  /* no buffer, only query the required length */
  ret = hwloc_snprintf(NULL, 0, "%s", "abcdef");
  if (ret != 6) {
    fprintf(stderr, "hwloc_snprintf() returned %d instead of 6 without buffer\n", ret);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
