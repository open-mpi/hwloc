/*
 * Copyright © 2009-2010 inria.  All rights reserved.
 * Copyright © 2009-2011 Université Bordeaux 1
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/autogen/config.h>
#include <hwloc.h>
#ifdef HWLOC_LINUX_SYS
#include <hwloc/linux.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <fcntl.h>

static int show_cpuset = 0;
static int logical = 1;

static void usage(char *name, FILE *where)
{
  fprintf (where, "Usage: %s [ options ] ...\n", name);
  fprintf (where, "Options:\n");
  fprintf (where, "  -a             Show all processes, including those that are not bound\n");
  fprintf (where, "  -l --logical   Use logical object indexes (default)\n");
  fprintf (where, "  -p --physical  Use physical object indexes\n");
  fprintf (where, "  -c --cpuset    Show cpuset instead of objects\n");
  fprintf (where, "  -t --threads   Show threads\n");
  fprintf (where, "  --whole-system Do not consider administration limitations\n");
}

static void print_task(hwloc_topology_t topology,
		       long pid, const char *name, hwloc_bitmap_t cpuset,
		       int thread)
{
  printf("%s%ld\t", thread ? " " : "", pid);

  if (show_cpuset) {
    char *cpuset_str = NULL;
    hwloc_bitmap_asprintf(&cpuset_str, cpuset);
    printf("%s", cpuset_str);
    free(cpuset_str);
  } else {
    hwloc_bitmap_t remaining = hwloc_bitmap_dup(cpuset);
    int first = 1;
    while (!hwloc_bitmap_iszero(remaining)) {
      char type[64];
      unsigned idx;
      hwloc_obj_t obj = hwloc_get_first_largest_obj_inside_cpuset(topology, remaining);
      hwloc_obj_type_snprintf(type, sizeof(type), obj, 1);
      idx = logical ? obj->logical_index : obj->os_index;
      if (idx == (unsigned) -1)
        printf("%s%s", first ? "" : " ", type);
      else
        printf("%s%s:%u", first ? "" : " ", type, idx);
      hwloc_bitmap_andnot(remaining, remaining, obj->cpuset);
      first = 0;
    }
    hwloc_bitmap_free(remaining);
  }

  printf("\t\t%s\n", name);
}

int main(int argc, char *argv[])
{
  const struct hwloc_topology_support *support;
  hwloc_topology_t topology;
  hwloc_const_bitmap_t topocpuset;
  hwloc_bitmap_t cpuset;
  unsigned long flags = 0;
  DIR *dir;
  struct dirent *dirent;
  int show_all = 0;
  int show_threads = 0;
  char *callname;
  int err;
  int opt;

  callname = strrchr(argv[0], '/');
  if (!callname)
    callname = argv[0];
  else
    callname++;

  while (argc >= 2) {
    opt = 0;
    if (!strcmp(argv[1], "-a"))
      show_all = 1;
    else if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--logical")) {
      logical = 1;
    } else if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--physical")) {
      logical = 0;
    } else if (!strcmp(argv[1], "-c") || !strcmp(argv[1], "--cpuset")) {
      show_cpuset = 1;
    } else if (!strcmp(argv[1], "-t") || !strcmp(argv[1], "--threads")) {
#ifdef HWLOC_LINUX_SYS
      show_threads = 1;
#else
      fprintf (stderr, "Listing threads is currently only supported on Linux\n");
#endif
    } else if (!strcmp (argv[1], "--whole-system")) {
      flags |= HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM;
    } else {
      fprintf (stderr, "Unrecognized option: %s\n", argv[1]);
      usage (callname, stderr);
      exit(EXIT_FAILURE);
    }
    argc -= opt+1;
    argv += opt+1;
  }

  err = hwloc_topology_init(&topology);
  if (err)
    goto out;

  hwloc_topology_set_flags(topology, flags);

  err = hwloc_topology_load(topology);
  if (err)
    goto out_with_topology;

  support = hwloc_topology_get_support(topology);

  if (!support->cpubind->get_thisproc_cpubind)
    goto out_with_topology;

  topocpuset = hwloc_topology_get_topology_cpuset(topology);

  dir  = opendir("/proc");
  if (!dir)
    goto out_with_topology;

  cpuset = hwloc_bitmap_alloc();
  if (!cpuset)
    goto out_with_dir;

  while ((dirent = readdir(dir))) {
    long pid;
    char *end;
    char name[64] = "";
    /* management of threads */
    unsigned boundthreads = 0, i;
    long *tids = NULL; /* NULL if process is not threaded */
    hwloc_bitmap_t *tidcpusets = NULL;

    pid = strtol(dirent->d_name, &end, 10);
    if (*end)
      /* Not a number */
      continue;

#ifdef HWLOC_LINUX_SYS
    {
      unsigned pathlen = 6 + strlen(dirent->d_name) + 1 + 7 + 1;
      char *path;
      int file;
      ssize_t n;

      path = malloc(pathlen);
      snprintf(path, pathlen, "/proc/%s/cmdline", dirent->d_name);
      file = open(path, O_RDONLY);
      free(path);

      if (file >= 0) {
        n = read(file, name, sizeof(name) - 1);
        close(file);

        if (n <= 0)
          /* Ignore kernel threads and errors */
          continue;

        name[n] = 0;
      }
    }
#endif /* HWLOC_LINUX_SYS */

    if (show_threads) {
#ifdef HWLOC_LINUX_SYS
      /* check if some threads must be displayed */
      unsigned pathlen = 6 + strlen(dirent->d_name) + 1 + 4 + 1;
      char *path;
      DIR *taskdir;

      path = malloc(pathlen);
      snprintf(path, pathlen, "/proc/%s/task", dirent->d_name);
      taskdir = opendir(path);
      if (taskdir) {
	struct dirent *taskdirent;
	long tid;
	char *end;
	unsigned n = 0;
	/* count threads */
	while ((taskdirent = readdir(taskdir))) {
	  tid = strtol(taskdirent->d_name, &end, 10);
	  if (*end)
	    /* Not a number */
	    continue;
	  n++;
	}
	if (n > 1) {
	  /* if there's more than one thread, see if some are bound */
	  tids = malloc(n * sizeof(*tids));
	  tidcpusets = calloc(n+1, sizeof(*tidcpusets));
	  if (tids && tidcpusets) {
	    /* reread the directory but gather info now */
	    rewinddir(taskdir);
	    i = 0;
	    while ((taskdirent = readdir(taskdir))) {
	      tid = strtol(taskdirent->d_name, &end, 10);
	      if (*end)
		/* Not a number */
		continue;
	      if (hwloc_linux_get_tid_cpubind(topology, tid, cpuset))
		continue;
	      hwloc_bitmap_and(cpuset, cpuset, topocpuset);
	      tids[i] = tid;
	      tidcpusets[i] = hwloc_bitmap_dup(cpuset);
	      i++;
	      if (hwloc_bitmap_iszero(cpuset))
		continue;
	      if (hwloc_bitmap_isequal(cpuset, topocpuset) && !show_all)
		continue;
	      boundthreads++;
	    }
	  } else {
	    /* failed to alloc, behave as if there were no threads */
	    free(tids); tids = NULL;
	    free(tidcpusets); tidcpusets = NULL;
	  }
	}
	closedir(taskdir);
      }
#endif /* HWLOC_LINUX_SYS */
    }

    if (hwloc_get_proc_cpubind(topology, pid, cpuset, 0))
      continue;

    hwloc_bitmap_and(cpuset, cpuset, topocpuset);
    if (hwloc_bitmap_iszero(cpuset))
      continue;

    /* don't print anything if the process isn't bound and if no threads are bound and if not showing all */
    if (hwloc_bitmap_isequal(cpuset, topocpuset) && (!tids || !boundthreads) && !show_all)
      continue;

    /* print the process */
    print_task(topology, pid, name, cpuset, 0);
    if (tids)
      /* print each tid we found (it's tidcpuset isn't NULL anymore) */
      for(i=0; tidcpusets[i] != NULL; i++) {
	print_task(topology, tids[i], "", tidcpusets[i], 1);
	hwloc_bitmap_free(tidcpusets[i]);
      }

    /* free threads stuff */
    free(tidcpusets);
    free(tids);
  }

  err = 0;
  hwloc_bitmap_free(cpuset);

 out_with_dir:
  closedir(dir);
 out_with_topology:
  hwloc_topology_destroy(topology);
 out:
  return err;
}
