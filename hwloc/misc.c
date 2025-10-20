/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright © 2009 CNRS
 * Copyright © 2009-2025 Inria.  All rights reserved.
 * Copyright © 2009-2010 Université Bordeaux
 * Copyright © 2009-2018 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "private/private.h"
#include "private/misc.h"

#include <stdarg.h>
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#ifdef HAVE_PROGRAM_INVOCATION_NAME
#include <errno.h>
extern char *program_invocation_name;
#endif
#ifdef HAVE___PROGNAME
extern char *__progname;
#endif

#ifndef HWLOC_HAVE_CORRECT_SNPRINTF
int hwloc_snprintf(char *str, size_t size, const char *format, ...)
{
  int ret;
  va_list ap;
  static char bin;
  size_t fakesize;
  char *fakestr;

  /* Some systems crash on str == NULL */
  if (!size) {
    str = &bin;
    size = 1;
  }

  va_start(ap, format);
  ret = vsnprintf(str, size, format, ap);
  va_end(ap);

  if (ret >= 0 && (size_t) ret != size-1)
    return ret;

  /* vsnprintf returned size-1 or -1. That could be a system which reports the
   * written data and not the actually required room. Try increasing buffer
   * size to get the latter. */

  fakesize = size;
  fakestr = NULL;
  do {
    fakesize *= 2;
    free(fakestr);
    fakestr = malloc(fakesize);
    if (NULL == fakestr)
      return -1;
    va_start(ap, format);
    errno = 0;
    ret = vsnprintf(fakestr, fakesize, format, ap);
    va_end(ap);
  } while ((size_t) ret == fakesize-1 || (ret < 0 && (!errno || errno == ERANGE)));

  if (ret >= 0 && size) {
    if (size > (size_t) ret+1)
      size = ret+1;
    memcpy(str, fakestr, size-1);
    str[size-1] = 0;
  }
  free(fakestr);

  return ret;
}
#endif

void hwloc_add_uname_info(struct hwloc_topology *topology __hwloc_attribute_unused,
			  void *cached_uname __hwloc_attribute_unused)
{
#ifdef HAVE_UNAME
  struct utsname _utsname, *utsname;

  if (hwloc_get_info_by_name(&topology->infos, "OSName"))
    /* don't annotate twice */
    return;

  if (cached_uname)
    utsname = (struct utsname *) cached_uname;
  else {
    utsname = &_utsname;
    if (uname(utsname) < 0)
      return;
  }

  if (*utsname->sysname)
    hwloc__add_info(&topology->infos, "OSName", utsname->sysname);
  if (*utsname->release)
    hwloc__add_info(&topology->infos, "OSRelease", utsname->release);
  if (*utsname->version)
    hwloc__add_info(&topology->infos, "OSVersion", utsname->version);
  if (*utsname->nodename)
    hwloc__add_info(&topology->infos, "HostName", utsname->nodename);
  if (*utsname->machine)
    hwloc__add_info(&topology->infos, "Architecture", utsname->machine);
#endif /* HAVE_UNAME */
}

void hwloc_fallback_add_pagesize_info(struct hwloc_topology *topology __hwloc_attribute_unused)
{
#ifdef hwloc_getpagesize
  int err;
  char buffer[42];
  const char *nrs;
  long pagesize;
  long largepagesize = -1; /* not found */

  if (hwloc_get_info_by_name(&topology->infos, "PageSizes"))
    /* don't annotate twice */
    return;

  pagesize = hwloc_getpagesize();
  largepagesize = -1; /* not found */
# if HAVE_DECL__SC_LARGE_PAGESIZE
  largepagesize = sysconf(_SC_LARGE_PAGESIZE);
# endif
  if (largepagesize == -1) {
    nrs = "1";
    err = snprintf(buffer, sizeof(buffer), "%ld", pagesize);
  } else {
    nrs = "2";
    err = snprintf(buffer, sizeof(buffer), "%ld,%ld", pagesize, largepagesize);
  }
  if (err < 0)
    return;
  hwloc__add_info(&topology->infos, "PageSizeNr", nrs);
  hwloc__add_info(&topology->infos, "PageSizes", buffer);
#endif /* hwloc_getpagesize */
}

static int hwloc_pagesize_arrayelt_compar(const void *_a, const void *_b)
{
  hwloc_pagesize_arrayelt_t a = *(hwloc_pagesize_arrayelt_t *) _a;
  hwloc_pagesize_arrayelt_t b = *(hwloc_pagesize_arrayelt_t *) _b;
  return a < b ? -1 : (a > b ? 1 : 0);
}

int hwloc__add_pagesize_info_from_array(struct hwloc_topology *topology,
                                        hwloc_pagesize_arrayelt_t *sizes,
                                        unsigned nr)
{
  size_t buflen, i;
  char nrs[11];
  char *buffer, *current;
  ssize_t err;

  /* might be needed on Linux when loading from sysfs dump (reordered dirents) or XML (just in case),
   * cheap anyway
   */
  qsort(sizes, nr, sizeof(*sizes), hwloc_pagesize_arrayelt_compar);

  buflen = 22*nr; /* 21 digits + comma or ending 0 */
  buffer = malloc(buflen);
  if (!buffer)
    return -1;

  current = buffer;
  for(i=0; i<nr; i++) {
    err = snprintf(current, buflen, "%s%llu", current != buffer ? "," : "", (unsigned long long) sizes[i]);
    if (err < 0)
      break;
    current += err;
    buflen -= err;
  }

  if (current != buffer) {
    snprintf(nrs, sizeof(nrs), "%u", nr);
    hwloc__add_info(&topology->infos, "PageSizeNr", nrs);
    hwloc__add_info(&topology->infos, "PageSizes", buffer);
  }

  free(buffer);
  return 0;
}

char *
hwloc_progname(struct hwloc_topology *topology __hwloc_attribute_unused)
{
#if (defined HAVE_DECL_GETMODULEFILENAME) && HAVE_DECL_GETMODULEFILENAME
  char name[256], *local_basename;
  unsigned res = GetModuleFileName(NULL, name, sizeof(name));
  if (res == sizeof(name) || !res)
    return NULL;
  local_basename = strrchr(name, '\\');
  if (!local_basename)
    local_basename = name;
  else
    local_basename++;
  return strdup(local_basename);
#else /* !HAVE_GETMODULEFILENAME */
  const char *name, *local_basename;
#if HAVE_DECL_GETPROGNAME
  name = getprogname(); /* FreeBSD, NetBSD, some Solaris */
#elif HAVE_DECL_GETEXECNAME
  name = getexecname(); /* Solaris */
#elif defined HAVE_PROGRAM_INVOCATION_NAME
  name = program_invocation_name; /* Glibc. */
  /* could use program_invocation_short_name directly, but we have the code to remove the path below anyway */
#elif defined HAVE___PROGNAME
  name = __progname; /* fallback for most unix, used for OpenBSD */
#else
  /* TODO: _NSGetExecutablePath(path, &size) on Darwin */
  /* TODO: AIX, HPUX */
  name = NULL;
#endif
  if (!name)
    return NULL;
  local_basename = strrchr(name, '/');
  if (!local_basename)
    local_basename = name;
  else
    local_basename++;
  return strdup(local_basename);
#endif /* !HAVE_GETMODULEFILENAME */
}
