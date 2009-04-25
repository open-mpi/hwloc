/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <config.h>

#define _ATFILE_SOURCE
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libtopology.h>
#include <libtopology/helper.h>
#include <libtopology/debug.h>

static void
lt_print_level_description(struct lt_level *l, FILE *output, int verbose_mode)
{
  unsigned long type = l->type;
  const char * separator = " + ";
  const char * current_separator = ""; /* not prefix for the first one */

#define lt_print_level_description_level(_l) \
    {									\
      if (type == _l) \
	{	      \
	  fprintf(output, "%s%s", current_separator, lt_level_string(_l)); \
	  current_separator = separator;				\
	}								\
    }

  lt_print_level_description_level(LT_LEVEL_MACHINE)
  lt_print_level_description_level(LT_LEVEL_FAKE)
  lt_print_level_description_level(LT_LEVEL_NODE)
  lt_print_level_description_level(LT_LEVEL_DIE)
  lt_print_level_description_level(LT_LEVEL_L3)
  lt_print_level_description_level(LT_LEVEL_L2)
  lt_print_level_description_level(LT_LEVEL_CORE)
  lt_print_level_description_level(LT_LEVEL_L1)
  lt_print_level_description_level(LT_LEVEL_PROC)
}

#define lt_memory_size_printf_value(_size) \
  (_size)%1024 ? (_size) : (_size)%(1024*1024) ? (_size)>>10 : (_size)>>20
#define lt_memory_size_printf_unit(_size) \
  (_size)%1024 ? "kB" : (_size)%(1024*1024) ? "MB" : "GB"

void
lt_print_level(lt_topo_t *topology, struct lt_level *l, FILE *output, int verbose_mode, const char *separator,
	       const char *indexprefix, const char* labelseparator, const char* levelterm)
{
  enum lt_level_e type = l->type;
  lt_print_level_description(l, output, verbose_mode);
  fprintf(output, "%s", labelseparator);
  switch (type) {
  case LT_LEVEL_DIE:
  case LT_LEVEL_CORE:
  case LT_LEVEL_PROC:
    fprintf(output, "%s%s%s%u", separator, lt_level_string(type), indexprefix, l->physical_index[type]);
    break;
  case LT_LEVEL_MACHINE:
    fprintf(output, "%s%s(%ld%s)", separator, lt_level_string(type),
	    lt_memory_size_printf_value(l->memory_kB[LT_LEVEL_MEMORY_MACHINE]),
	    lt_memory_size_printf_unit(l->memory_kB[LT_LEVEL_MEMORY_MACHINE]));
    break;
  case LT_LEVEL_NODE:
  case LT_LEVEL_L3:
  case LT_LEVEL_L2:
  case LT_LEVEL_L1: {
    enum lt_level_memory_type_e mtype =
      (type == LT_LEVEL_NODE) ? LT_LEVEL_MEMORY_NODE
      : (type == LT_LEVEL_L3) ? LT_LEVEL_MEMORY_L3
      : (type == LT_LEVEL_L2) ? LT_LEVEL_MEMORY_L2
      : LT_LEVEL_MEMORY_L1;
    unsigned long memory_kB = l->memory_kB[mtype];
    fprintf(output, "%s%s%s%u(%ld%s)", separator, lt_level_string(type), indexprefix, l->physical_index[type],
	    lt_memory_size_printf_value(memory_kB),
	    lt_memory_size_printf_unit(memory_kB));
    break;
  }
  default:
    break;
  }
  if (l->level == topology->nb_levels-1) {
    fprintf(output, "%sVP %s%u", separator, indexprefix, l->number);
  }
  fprintf(output,"%s\n", levelterm);
}


#ifdef HAVE_OPENAT

static int
lt_set_fsys_root(lt_topo_t *topology, const char *path)
{
  int root;

  root = open(path, O_RDONLY | O_DIRECTORY);
  if (root < 0)
    return -1;

  topology->fsys_root_fd = root;
  return 0;
}

FILE *
lt_fopenat(const char *path, const char *mode, int fsys_root_fd)
{
  int fd;
  const char *relative_path;

  assert(!(fsys_root_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  fd = openat (fsys_root_fd, relative_path, O_RDONLY);
  if (fd == -1)
    return NULL;

  return fdopen(fd, mode);
}

int
lt_accessat(const char *path, int mode, int fsys_root_fd)
{
  const char *relative_path;

  assert(!(fsys_root_fd < 0));

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  return faccessat(fsys_root_fd, relative_path, O_RDONLY, 0);
}

DIR*
lt_opendirat(const char *path, int fsys_root_fd)
{
  int dir_fd;
  const char *relative_path;

  /* Skip leading slashes.  */
  for (relative_path = path; *relative_path == '/'; relative_path++);

  dir_fd = openat(fsys_root_fd, relative_path, O_RDONLY | O_DIRECTORY);
  if (dir_fd < 0)
    return NULL;

  return fdopendir(dir_fd);
}

#endif /* !HAVE_OPENAT */


/* Return the OS-provided number of processors.  Unlike other methods such as
   reading sysfs on Linux, this method is not virtualizable; thus it's only
   used as a fall-back method, allowing `lt_set_fsys_root ()' to
   have the desired effect.  */
static unsigned
lt_fallback_nbprocessors(void) {
	/* TODO: change into HAVE_SC_foobar, for OSes that don't provide the macro version */
#if defined(_SC_NPROCESSORS_ONLN)
  return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_SC_NPROCESSORS_CONF)
  return sysconf(_SC_NPROCESSORS_CONF);
#elif defined(_SC_NPROC_CONF) || defined(IRIX_SYS)
  return sysconf(_SC_NPROC_CONF);
	/* TODO: change into HAVE_HOST_INFO */
#elif defined(DARWIN_SYS)
  struct host_basic_info info;
  mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
  host_info(mach_host_self(), HOST_BASIC_INFO, (integer_t*) &info, &count);
  return info.avail_cpus;
#else
#warning No known way to discover number of available processors on this system
#warning lt_fallback_nbprocessors will default to 1
  return 1;
#endif
}


const char *
lt_level_string(enum lt_level_e l)
{
  switch (l)
    {
    case LT_LEVEL_MACHINE: return "Machine";
    case LT_LEVEL_FAKE: return "Fake";
    case LT_LEVEL_NODE: return "NUMANode";
    case LT_LEVEL_DIE: return "Die";
    case LT_LEVEL_L3: return "L3Cache";
    case LT_LEVEL_L2: return "L2Cache";
    case LT_LEVEL_CORE: return "Core";
    case LT_LEVEL_L1: return "L1Cache";
    case LT_LEVEL_PROC: return "SMTproc";
    default: return "Unknown";
    }
}


/* is it really useful to try to disable these two not so big static functions?
 */
#if defined(LINUX_SYS) || defined(HAVE_LIBKSTAT)
void
lt_setup_die_level(int procid_max, unsigned numdies, unsigned *osphysids, unsigned *proc_physids, lt_topo_t *topology)
{
  struct lt_level *die_level;
  int j;

  ltdebug("%d dies\n", numdies);
  die_level=malloc((numdies+1)*sizeof(*die_level));
  assert(die_level);

  for (j = 0; j < numdies; j++)
    {
      lt_setup_level(&die_level[j], LT_LEVEL_DIE);
      lt_set_os_numbers(&die_level[j], LT_LEVEL_DIE, osphysids[j]);
      lt_level_cpuset_from_array(&die_level[j], j, proc_physids, procid_max);
      ltdebug("die %d has cpuset %"LT_PRIxCPUSET"\n",
	      j, LT_CPUSET_PRINTF_VALUE(die_level[j].cpuset));
    }
  ltdebug("\n");

  lt_cpuset_zero(&die_level[j].cpuset);

  topology->level_nbitems[topology->nb_levels]=numdies;
  ltdebug("--- die level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=die_level;
  ltdebug("\n");
}

void
lt_setup_core_level(int procid_max, unsigned numcores, unsigned *oscoreids, unsigned *proc_coreids, lt_topo_t *topology)
{
  struct lt_level *core_level;
  int j;

  ltdebug("%d cores\n", numcores);
  core_level=malloc((numcores+1)*sizeof(*core_level));
  assert(core_level);

  for (j = 0; j < numcores; j++)
    {
      lt_setup_level(&core_level[j], LT_LEVEL_CORE);
      lt_set_os_numbers(&core_level[j], LT_LEVEL_CORE, oscoreids[j]);
      lt_level_cpuset_from_array(&core_level[j], j, proc_coreids, procid_max);
      ltdebug("core %d has cpuset %"LT_PRIxCPUSET"\n",
	      j, LT_CPUSET_PRINTF_VALUE(core_level[j].cpuset));
    }

  ltdebug("\n");

  lt_cpuset_zero(&core_level[j].cpuset);

  topology->level_nbitems[topology->nb_levels]=numcores;
  ltdebug("--- core level has number %d\n", topology->nb_levels);
  topology->levels[topology->nb_levels++]=core_level;
  ltdebug("\n");
}
#endif /* LINUX_SYS || HAVE_LIBKSTAT */

#if defined(LINUX_SYS)
void
lt_setup_cache_level(int cachelevel, enum lt_level_e topotype, int procid_max,
		     unsigned *numcaches, unsigned *cacheids, unsigned long *cachesizes,
		     lt_topo_t *topology)
{
  struct lt_level *level;
  int j;

  ltdebug("%d L%d caches\n", numcaches[cachelevel], cachelevel+1);
  level=malloc((numcaches[cachelevel]+1)*sizeof(*level));
  assert(level);

  for (j = 0; j < numcaches[cachelevel]; j++)
    {
      lt_setup_level(&level[j], topotype);

      switch (cachelevel)
	{
	case 2: lt_set_os_numbers(&level[j], LT_LEVEL_L3, j); break;
	case 1: lt_set_os_numbers(&level[j], LT_LEVEL_L2, j); break;
	case 0: lt_set_os_numbers(&level[j], LT_LEVEL_L1, j); break;
	default: assert(!1);
	}

      level[j].memory_kB[LT_LEVEL_MEMORY_L1+cachelevel] = cachesizes[cachelevel*LIBTOPO_NBMAXCPUS+j];

      lt_level_cpuset_from_array(&level[j], j, &cacheids[cachelevel*LIBTOPO_NBMAXCPUS], procid_max);

      ltdebug("L%d cache %d has cpuset %"LT_PRIxCPUSET"\n",
	      cachelevel+1, j, LT_CPUSET_PRINTF_VALUE(level[j].cpuset));
    }
  ltdebug("\n");
  lt_cpuset_zero(&level[j].cpuset);
  topology->level_nbitems[topology->nb_levels]=numcaches[cachelevel];
  ltdebug("--- shared L%d level has number %d\n", cachelevel+1, topology->nb_levels);
  topology->levels[topology->nb_levels++]=level;
  ltdebug("\n");
}
#endif /* LINUX_SYS */

#ifdef OSF_SYS
#       include <numa.h>
/* Ask libnuma for topology */
static void
look_osf(lt_topo_t *topology)
{
  cpu_cursor_t cursor;
  unsigned i = 0;
  unsigned nbnodes;
  radid_t radid;
  cpuid_t cpuid;
  cpuset_t cpuset;
  struct lt_level *node_level;

  nbnodes=rad_get_num();
  if (nbnodes==1)
    return;

  node_level=malloc((nbnodes+1)*sizeof(*node_level));

  cpusetcreate(&cpuset);
  for (radid = 0; radid < nbnodes; radid++) {
    cpuemptyset(cpuset);
    if (rad_get_cpus(radid, cpuset)==-1) {
      fprintf(stderr,"rad_get_cpus(%d) failed: %s\n",radid,strerror(errno));
      continue;
    }

    lt_setup_level(&node_level[i], LT_LEVEL_NODE);
    lt_set_os_numbers(&node_level[i], node, radid);
    node_level[i].memory_kB[LT_LEVEL_MEMORY_NODE] = 0; /* TODO */
    node_level[i].huge_page_free = 0;

    cursor = SET_CURSOR_INIT;
    while((cpuid = cpu_foreach(cpuset, 0, &cursor)) != CPU_NONE)
      lt_cpuset_set(&node_level[i].cpuset,cpuid);

    ltdebug("node %d has cpuset %"LT_PRIxCPUSET"\n",
	   i, LT_CPUSET_PRINTF_VALUE(node_level[i].cpuset));
    i++;
  }

  lt_cpuset_zero(&node_level[i].cpuset);

  topology->level_nbitems[topology->nb_levels] = nbnodes;
  topology->levels[topology->nb_levels++] = node_level;
}
#endif /* OSF_SYS */


#ifdef AIX_SYS
#      include <sys/rset.h>

/* Ask rsets for topology */
static void
look_rset(int sdl, enum lt_level_e level, lt_topo_t *topology)
{
  rsethandle_t rset, rad;
  int r,i,maxcpus,j;
  unsigned nbnodes;
  struct lt_level *rad_level;

  rset = rs_alloc(RS_PARTITION);
  rad = rs_alloc(RS_EMPTY);
  nbnodes = rs_numrads(rset, sdl, 0);
  if (nbnodes == -1) {
    perror("rs_numrads");
    return;
  }
  if (nbnodes == 1)
    return;

  rad_level=malloc((nbnodes+1)*sizeof(*rad_level));

  for (r = 0, i = 0; i < nbnodes; i++) {
    if (rs_getrad(rset, rad, sdl, i, 0)) {
      fprintf(stderr,"rs_getrad(%d) failed: %s\n", i, strerror(errno));
      continue;
    }
    if (!rs_getinfo(rad, R_NUMPROCS, 0))
      continue;

    lt_setup_level(&rad_level[r], level);
    lt_set_os_numbers(&rad_level[r], level, r);
    switch(level) {
      case LT_LEVEL_NODE:
	rad_level[r].memory_kB[LT_LEVEL_MEMORY_NODE] = 0; /* TODO */
	rad_level[r].huge_page_free = 0;
	break;
      case LT_LEVEL_L2:
	rad_level[r].memory_kB[LT_LEVEL_MEMORY_L2] = 0; /* TODO */
	break;
      default:
	break;
    }
    maxcpus = rs_getinfo(rad, R_MAXPROCS, 0);
    for (j = 0; j < maxcpus; j++) {
      if (rs_op(RS_TESTRESOURCE, rad, NULL, R_PROCS, j))
	lt_cpuset_set(&rad_level[r].cpuset,j);
    }
    ltdebug("node %d has cpuset %"LT_PRIxCPUSET"\n",
	   r, LT_CPUSET_PRINTF_VALUE(rad_level[r].cpuset));
    r++;
  }

  lt_cpuset_zero(&rad_level[r].cpuset);

  topology->level_nbitems[topology->nb_levels] = nbnodes;
  topology->levels[topology->nb_levels++] = rad_level;
  rs_free(rset);
  rs_free(rad);
}

static void
look_aix(lt_topo_t *topology)
{
  unsigned i;
  for (i=0; i<=rs_getinfo(NULL, R_MAXSDL, 0); i++)
    {
      if (i == rs_getinfo(NULL, R_MCMSDL, 0))
	{
	  ltdebug("looking AIX node sdl %d\n",i);
	  look_rset(i, LT_LEVEL_NODE, topology);
	}
#      ifdef R_L2CSDL
      if (i == rs_getinfo(NULL, R_L2CSDL, 0))
	{
	  ltdebug("looking AIX L2 sdl %d\n",i);
	  look_rset(i, LT_LEVEL_L2, topology);
	}
#      endif
#      ifdef R_PCORESDL
      if (i == rs_getinfo(NULL, R_PCORESDL, 0))
	{
	  ltdebug("looking AIX core sdl %d\n",i);
	  look_rset(i, LT_LEVEL_CORE, topology);
	}
#      endif
      if (i == rs_getinfo(NULL, R_SMPSDL, 0))
	ltdebug("not looking AIX \"SMP\" sdl %d\n",i);
      if (i == rs_getinfo(NULL, R_MAXSDL, 0))
	{
	  ltdebug("looking AIX max sdl %d\n",i);
	  look_rset(i, LT_LEVEL_PROC, topology);
	}
    }
}
#endif /* AIX_SYS */


#ifdef WIN_SYS
#  warning: TODO: use GetLogicalProcessorInformation, GetNumaHighestNodeNumber, and GetNumaNodeProcessorMask
#endif


/* Use the value stored in topology->nb_processors.  */
static void
look_cpu(lt_cpuset_t *offline_cpus_set, lt_topo_t *topology)
{
  struct lt_level *cpu_level;
  unsigned oscpu,cpu;

  cpu_level=malloc((topology->nb_processors+1)*sizeof(*cpu_level));
  assert(cpu_level);

  ltdebug("\n\n * CPU cpusets *\n\n");
  for (cpu=0,oscpu=0; cpu<topology->nb_processors; cpu++,oscpu++)
    {
      while (lt_cpuset_isset(offline_cpus_set, oscpu))
	oscpu++;
      lt_setup_level(&cpu_level[cpu], LT_LEVEL_PROC);
      lt_set_os_numbers(&cpu_level[cpu], LT_LEVEL_PROC, oscpu);

      lt_cpuset_cpu(&cpu_level[cpu].cpuset, oscpu);

      ltdebug("cpu %d (os %d) has cpuset %"LT_PRIxCPUSET"\n",
	      cpu, oscpu, LT_CPUSET_PRINTF_VALUE(cpu_level[cpu].cpuset));
    }
  lt_cpuset_zero(&cpu_level[cpu].cpuset);

  topology->level_nbitems[topology->nb_levels]=topology->nb_processors;
  topology->levels[topology->nb_levels++]=cpu_level;
}


/* Connect levels */
static void
topo_connect(lt_topo_t *topology)
{
  int l, i, j, m;
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
	{
	  if (topology->levels[l][i].arity)
	    {
	      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
		      l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	      topology->levels[l][i].children=malloc(topology->levels[l][i].arity*sizeof(void *));
	      assert(topology->levels[l][i].children);

	      m=0;
	      for (j=0; !lt_cpuset_iszero(&topology->levels[l+1][j].cpuset); j++)
		if (lt_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
		  {
		    assert(!(m >= topology->levels[l][i].arity));
		    topology->levels[l][i].children[m] = &topology->levels[l+1][j];
		    topology->levels[l+1][j].father = &topology->levels[l][i];
		    topology->levels[l+1][j].index = m++;
		  }
	    }
	}
    }
}

static int
compar(const void *_l1, const void *_l2)
{
  const struct lt_level *l1 = _l1, *l2 = _l2;
  return lt_cpuset_first(&l1->cpuset) - lt_cpuset_first(&l2->cpuset);
}

/* Main discovery loop */
static void
topo_discover(lt_topo_t *topology)
{
  unsigned l,i=0,j;

  assert(topology!=NULL);

  /* Initialize the number of processor to some reasonable default, e.g.,
     obtained using sysconf(3).  */
  topology->nb_processors = lt_fallback_nbprocessors ();

  /* Raw detection, from coarser levels to finer levels */
  unsigned k;
  /*	unsigned nbsublevels; */
  /*	unsigned sublevelarity; */
  lt_cpuset_t offline_cpus_set;

  lt_cpuset_zero(&offline_cpus_set);

#    ifdef LINUX_SYS
  look_linux(topology, &offline_cpus_set);
#    endif /* LINUX_SYS */

#    ifdef  AIX_SYS
  look_aix(topology);
#    endif /* AIX_SYS */

#    ifdef  OSF_SYS
  look_osf(topology);
#    endif /* OSF_SYS */

#    ifdef HAVE_LIBLGRP
  look_lgrp(topology);
#    endif /* HAVE_LIBLGRP */
#    ifdef HAVE_LIBKSTAT
  look_kstat(topology);
#    endif /* HAVE_LIBKSTAT */

  look_cpu(&offline_cpus_set, topology);

  ltdebug("\n\n--> discovered %d levels\n\n", topology->nb_levels);

  assert(topology->nb_processors);

  /* Compute the machine cpuset */
  lt_cpuset_zero(&topology->levels[0][0].cpuset);
  for (i=0; i<topology->level_nbitems[1]; i++)
    lt_cpuset_orset(&topology->levels[0][0].cpuset, &topology->levels[1][i].cpuset);

  /* sort levels according to cpu sets */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      /* first sort sublevels according to cpu sets */
      qsort(&topology->levels[l+1][0], topology->level_nbitems[l+1], sizeof(*topology->levels[l+1]), compar);
      k = 0;
      /* then gather sublevels according to levels */
      for (i=0; i<topology->level_nbitems[l]; i++)
	{
	  lt_cpuset_t level_set = topology->levels[l][i].cpuset;
	  for (j=k; j<topology->level_nbitems[l+1]; j++)
	    {
	      lt_cpuset_t set = level_set;
	      lt_cpuset_andset(&set, &topology->levels[l+1][j].cpuset);
	      if (!lt_cpuset_iszero(&set))
		{
		  /* Sublevel j is part of level i, put it at k.  */
		  struct lt_level level = topology->levels[l+1][j];
		  memmove(&topology->levels[l+1][k+1], &topology->levels[l+1][k], (j-k)*sizeof(*topology->levels[l+1]));
		  topology->levels[l+1][k++] = level;
		}
	    }
	}
    }

  /* Now we can put numbers on levels. */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      {
	topology->levels[l][i].number = i;
	ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));
      }

  /* And show debug again */
  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET"\n", l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset));

  /* Compute arity */
  for (l=0; l+1<topology->nb_levels; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
	{
	  topology->levels[l][i].arity=0;
	  for (j=0; !lt_cpuset_iszero(&topology->levels[l+1][j].cpuset); j++)
	    if (lt_cpuset_isincluded(&topology->levels[l][i].cpuset, &topology->levels[l+1][j].cpuset))
	      topology->levels[l][i].arity++;
	  ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" arity %u\n",
		  l, i, LT_CPUSET_PRINTF_VALUE(topology->levels[l][i].cpuset), topology->levels[l][i].arity);
	}
    }


  for (i=0; !lt_cpuset_iszero(&topology->levels[topology->nb_levels-1][i].cpuset); i++)
    ltdebug("level %u,%u: cpuset %"LT_PRIxCPUSET" leaf\n",
	    topology->nb_levels-1, i, LT_CPUSET_PRINTF_VALUE(topology->levels[topology->nb_levels-1][i].cpuset));
  ltdebug("arity done.\n");

  /* and finally connect levels */
  topo_connect(topology);
  ltdebug("connecting done.\n");

  /* intialize all depth to unknown */
  for (l=0; l < LT_LEVEL_MAX; l++)
    topology->type_depth[l] = -1;

  /* walk the existing levels to set their depth */
  for (l=0; l<topology->nb_levels; l++)
    topology->type_depth[topology->levels[l][0].type] = l;

  /* setup the depth of all still unknown levels (the one that got merged or never created */
  int type, prevdepth = -1;
  for (type = 0; type < LT_LEVEL_MAX; type++)
    {
      if (topology->type_depth[type] == -1)
	topology->type_depth[type] = prevdepth;
      else
	prevdepth = topology->type_depth[type];
    }

  for (l=0; l<topology->nb_levels; l++)
    for (i=0; i<topology->level_nbitems[l]; i++)
      topology->levels[l][i].level = l;
}

static void
lt_get_dmi_info(struct lt_topo *topology)
{
#ifdef LINUX_SYS
#define DMI_BOARD_STRINGS_LEN 50
  char dmi_line[DMI_BOARD_STRINGS_LEN];
  FILE *fd;

  dmi_line[0] = '\0';
  fd = lt_fopen("/sys/class/dmi/id/board_vendor", "r", topology->fsys_root_fd);
  if (fd) {
    fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (dmi_line[0] != '\0') {
      topology->dmi_board_vendor = strdup(dmi_line);
      ltdebug("found DMI board vendor '%s'\n", topology->dmi_board_vendor);
    }
  }

  dmi_line[0] = '\0';
  fd = lt_fopen("/sys/class/dmi/id/board_name", "r", topology->fsys_root_fd);
  if (fd) {
    fgets(dmi_line, DMI_BOARD_STRINGS_LEN, fd);
    fclose (fd);
    if (dmi_line[0] != '\0') {
      topology->dmi_board_name = strdup(dmi_line);
      ltdebug("found DMI board name '%s'\n", topology->dmi_board_name);
    }
  }
#endif /* LINUX_SYS */
}

int
lt_topo_init (struct lt_topo **topologyp, const char *fsys_root_path)
{
  struct lt_topo *topology;
#ifdef HAVE_OPENAT
  char *fsys_root_path_env;
#endif

  topology = topo_default_allocator.allocate (sizeof (struct lt_topo));
  if(!topology)
    return -1;

  lt_setup_topo(topology);

#ifdef HAVE_OPENAT
  /* Use the root path from the environment variable first,
   * then from the given argument, then the default root.
   */
  fsys_root_path_env = getenv("LT_FSYS_ROOT_PATH");
  if (fsys_root_path_env)
    fsys_root_path = fsys_root_path_env;
  if (!fsys_root_path)
    fsys_root_path = "/";

  /* Get a file descriptor to the file system root.  */
  if (lt_set_fsys_root(topology, fsys_root_path)) {
    perror ("opening file system root");
    assert(0);
  }
#endif

  topo_discover(topology);

  topology->dmi_board_vendor = topology->dmi_board_name = NULL;
  lt_get_dmi_info(topology);

  *topologyp = topology;
  return 1;
}

void
lt_topo_fini (struct lt_topo *topology)
{
  unsigned l,i;
  /* Last level is not freed because we need it in various places */
  for (l=0; l<topology->nb_levels-1; l++)
    {
      for (i=0; !lt_cpuset_iszero(&topology->levels[l][i].cpuset); i++)
	{
	  free(topology->levels[l][i].children);
	  topology->levels[l][i].children = NULL;
	}
      if (l)
	{
	  free(topology->levels[l]);
	  topology->levels[l] = NULL;
	}
    }
  free(topology->dmi_board_vendor);
  free(topology->dmi_board_name);
}
