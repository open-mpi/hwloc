/*
 * Copyright © 2009 CNRS
 * Copyright © 2009-2019 Inria.  All rights reserved.
 * Copyright © 2009-2010 Université Bordeaux
 * Copyright © 2009-2011 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "misc.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

char *callname;
hwloc_topology_t topology;

void usage(const char *_callname __hwloc_attribute_unused, FILE *where)
{
  fprintf(where, "Usage: hwloc-distrib round-robin <type> [options]\n");
  fprintf(where, "       hwloc-distrib scatter <type> [options]\n");
  fprintf(where, "       hwloc-distrib <type:type:...:type> [options]\n");

  fprintf(where, "Distribution options:\n");
  fprintf(where, "  --ignore <type>         Ignore objects of the given type\n");
  fprintf(where, "  -n <number>             Distribute <number> objects. Cycle if there is less than <number> objects.\n");
  fprintf(where, "  --reverse               Distribute by starting from last objects\n");
  fprintf(where, "  --shuffle               Stick to distribution policy but shuffle indexes inside levels.\n");
  fprintf(where, "  --from <logical_index>  Logical index of the first object of type to distribute.\n");

  fprintf(where, "Input topology options:\n");
  fprintf(where, "  --restrict <set>        Restrict the topology to processors listed in <set>\n");
  fprintf(where, "  --disallowed            Include objects disallowed by administrative limitations\n");
  hwloc_utils_input_format_usage(where, 0);
  fprintf(where, "Formatting options:\n");
  fprintf(where, "  --single                Singlify each output to a single CPU\n");
  fprintf(where, "  --taskset               Show taskset-specific cpuset strings\n");
  fprintf(where, "  --logical-index         Show objects logical index\n");
  fprintf(where, "  --physical-index        Show objects os index\n");
  fprintf(where, "Miscellaneous options:\n");
  fprintf(where, "  -v --verbose            Show verbose messages\n");
  fprintf(where, "  --version               Report version and exit\n");
}

#define ROUND_ROBIN 0
#define SCATTER     1
#define CUSTOM      2
char             *arg_types; // argv containing types to parse
int               policy; // policy among ROUND_ROBIN, SCATTER, CUSTOM.
hwloc_obj_type_t *policy_types = NULL; // resulting types after parsing arg_types
int               num_types=1; // The number of parsed types in policy_types.

static hwloc_obj_type_t parse_policy_type(const char* type){
	int depth;
	hwloc_obj_t obj;
	
	if (hwloc_type_sscanf_as_depth(type, NULL, topology, &depth) < 0) {
		fprintf(stderr, "Unrecognized type `%s'.\n", type);
		exit(EXIT_FAILURE);
	}
	if (depth < 0){
		fprintf(stderr, "Unsupported policy type `%s' with negative depth.\n", type);
		exit(EXIT_FAILURE);
	}
	obj = hwloc_get_obj_by_depth(topology, depth, 0);
	assert(obj != NULL);

	return obj->type;
}

// Parse string in arg_types after topology create, load, filter etc...
static void parse_policy(void){
	if (policy == ROUND_ROBIN){
		num_types = 1;
		policy_types = malloc(sizeof(*policy_types));
		*policy_types = parse_policy_type(arg_types);
	}
	else if (policy == SCATTER){
		num_types = 1;
		policy_types = malloc(sizeof(*policy_types));
		*policy_types = parse_policy_type(arg_types);
	}
	else {
		size_t i;
		char *type;
		
		for(i=0; i<strlen(arg_types); i++)
			if (arg_types[i] == ':')
				num_types++;
		policy_types = malloc(sizeof(*policy_types) * num_types);

		i=0;
		type=strtok(arg_types, ":");
		while(type != NULL){
			policy_types[i++] = parse_policy_type(type);
			type=strtok(NULL, ":");
		}
	}
}

int main(int argc, char *argv[])
{
  long n = -1;
  char *input = NULL;
  enum hwloc_utils_input_format input_format = HWLOC_UTILS_INPUT_DEFAULT;
  int taskset = 0;
  int singlify = 0;
  int logical_index = 0;
  int physical_index = 0;    
  int verbose = 0;
  char *restrictstring = NULL;
  int from_index = -1;
  unsigned long flags = 0;
  unsigned long dflags = 0;
  int opt;
  int err;

  callname = argv[0];
  hwloc_utils_check_api_version(callname);
  
  /* skip argv[0], handle options */
  argv++;
  argc--;

  /* Prepare for parsing policy option */  
  if (argc < 1) {
	  usage(callname, stdout);
	  return EXIT_SUCCESS;
  }
  if (!strcmp(argv[0], "round-robin") ){
	  if (argc<2) {
		  fprintf(stderr, "round-robin policy requires a type argument.\n");
		  return EXIT_FAILURE;
	  }
	  arg_types = argv[1];
	  policy = ROUND_ROBIN;
	  argv++; argv++;
	  argc-=2;	  
  }
  else if (!strcmp(argv[0], "scatter")){
	  if (argc<2) {
		  fprintf(stderr, "scatter policy requires a type argument.\n");
		  return EXIT_FAILURE;
	  }
	  arg_types = argv[1];
	  policy = SCATTER;
	  argv++; argv++;
	  argc-=2;
  }
  else {
	  arg_types = argv[0];
	  policy = CUSTOM;
	  argv++;
	  argc--;
  }

  /* enable verbose backends */
  putenv((char *) "HWLOC_XML_VERBOSE=1");
  putenv((char *) "HWLOC_SYNTHETIC_VERBOSE=1");

  hwloc_topology_init(&topology);

  while (argc >= 1) {
    if (!strcmp(argv[0], "--")) {
      argc--;
      argv++;
      break;
    }
    if (*argv[0] == '-') {
      if (!strcmp(argv[0], "--single")) {
	singlify = 1;
	goto next;
      }
      if (!strcmp(argv[0], "--taskset")) {
	taskset = 1;
	goto next;
      }
      if (!strcmp(argv[0], "--logical-index")) {
	logical_index = 1;
	goto next;
      }      
      if (!strcmp(argv[0], "--physical-index")) {
	physical_index = 1;
	goto next;
      }
      if (!strcmp(argv[0], "-v") || !strcmp(argv[0], "--verbose")) {
	verbose = 1;
	goto next;
      }
      if (!strcmp (argv[0], "--disallowed") || !strcmp (argv[0], "--whole-system")) {
	flags |= HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED;
	goto next;
      }
      if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help")) {
	usage(callname, stdout);
	return EXIT_SUCCESS;
      }
      if (hwloc_utils_lookup_input_option(argv, argc, &opt,
					  &input, &input_format,
					  callname)) {
	argv += opt;
	argc -= opt;
	goto next;
      }
      else if (!strcmp (argv[0], "--ignore")) {
	hwloc_obj_type_t type;
	if (argc < 2) {
	  usage(callname, stdout);
	  exit(EXIT_FAILURE);
	}
	if (hwloc_type_sscanf(argv[1], &type, NULL, 0) < 0)
	  fprintf(stderr, "Unsupported type `%s' passed to --ignore, ignoring.\n", argv[1]);
	else
	  hwloc_topology_set_type_filter(topology, type, HWLOC_TYPE_FILTER_KEEP_NONE);
	argc--;
	argv++;
	goto next;
      }
      else if (!strcmp (argv[0], "--from")) {
	if (argc < 2) {
	  usage(callname, stdout);
	  exit(EXIT_FAILURE);
	}
	from_index = atoi(argv[1]);
	argc--;
	argv++;
	goto next;
      }
      else if (!strcmp (argv[0], "--reverse")) {
	dflags |= HWLOC_DISTRIB_FLAG_REVERSE;
	goto next;
      }
      else if (!strcmp (argv[0], "--shuffle")) {
	dflags |= HWLOC_DISTRIB_FLAG_SHUFFLE;
	goto next;
      }
      else if (!strcmp (argv[0], "--restrict")) {
	if (argc < 2) {
	  usage (callname, stdout);
	  exit(EXIT_FAILURE);
	}
	restrictstring = strdup(argv[1]);
	argc--;
	argv++;
	goto next;
      }
      else if (!strcmp (argv[0], "-n")) {
	if (argc < 2) {
	  usage (callname, stdout);
	  exit(EXIT_FAILURE);
	}
	n = atol(argv[1]);
	argc--;
	argv++;
	goto next;
      }
      else if (!strcmp (argv[0], "--version")) {
          printf("%s %s\n", callname, HWLOC_VERSION);
          exit(EXIT_SUCCESS);
      }

      fprintf (stderr, "Unrecognized option: %s\n", argv[0]);
      usage(callname, stderr);
      return EXIT_FAILURE;
    }

  next:
    argc--;
    argv++;
  }

  {
    hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
    struct hwloc_distrib_iterator *it;
    hwloc_obj_t root, next=NULL;
    
    if (input) {
      err = hwloc_utils_enable_input_format(topology,
					    input,
					    &input_format,
					    verbose,
					    callname);
      if (err) {
	free(cpuset);
	return EXIT_FAILURE;
      }
    }
    hwloc_topology_set_flags(topology, flags);
    err = hwloc_topology_load(topology);
    if (err < 0) {
      free(cpuset);
      return EXIT_FAILURE;
    }

    if (restrictstring) {
      hwloc_bitmap_t restrictset = hwloc_bitmap_alloc();
      hwloc_bitmap_sscanf(restrictset, restrictstring);
      err = hwloc_topology_restrict (topology, restrictset, 0);
      if (err) {
	perror("Restricting the topology");
	/* FALLTHRU */
      }
      hwloc_bitmap_free(restrictset);
      free(restrictstring);
    }

    root = hwloc_get_obj_by_depth(topology, 0, 0);
    parse_policy();
    if (policy == ROUND_ROBIN){
	    it = hwloc_distrib_iterator_round_robin(topology, *policy_types, dflags);
    } else if (policy == SCATTER){
	    it = hwloc_distrib_iterator_scatter(topology, *policy_types, dflags);
    } else {
	    it = hwloc_distrib_build_iterator(topology,
					      &root,
					      1,
					      policy_types,
					      num_types,
					      dflags);
    }
    if (it == NULL)
	    return EXIT_FAILURE;

    // Go to start index.
    while ( hwloc_distrib_iterator_next(topology, it, &next) &&
	    from_index > 0 && next->logical_index != from_index );

    int continue_it = 1;
    do {
	    if (logical_index) {
		    printf("%d\n", next->logical_index);
	    } else if (physical_index){
		    printf("%d\n", next->os_index);
	    } else {
		    hwloc_bitmap_copy(cpuset, next->cpuset);
		    char *str = NULL;
		    if (singlify) {
			    if (dflags & HWLOC_DISTRIB_FLAG_REVERSE) {
				    unsigned last = hwloc_bitmap_last(cpuset);
				    hwloc_bitmap_only(cpuset, last);
			    } else {
				    hwloc_bitmap_singlify(cpuset);
			    }
		    }
		    if (taskset)
			    hwloc_bitmap_taskset_asprintf(&str, cpuset);
		    else
			    hwloc_bitmap_asprintf(&str, cpuset);
		    printf("%s\n", str);
		    free(str);
	    }
	    if ((! continue_it && n < 0) || --n == 0)
		    break;
	    continue_it = hwloc_distrib_iterator_next(topology, it, &next);
    } while (1);
    hwloc_bitmap_free(cpuset);
    hwloc_distrib_destroy_iterator(it);
    free(policy_types);
  }
  
  hwloc_topology_destroy(topology);

  return EXIT_SUCCESS;
}
