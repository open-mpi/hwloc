/*
 * Copyright © 2012-2020 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc-calc.h"
#include "hwloc.h"
#include "misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void usage(const char *callname __hwloc_attribute_unused, FILE *where)
{
	fprintf(where, "Usage: hwloc-annotate [options] <input.xml> <output.xml> -- <location1> <location2> ... -- <annotation>\n");
	fprintf(where, "Usage: hwloc-annotate [options] <input.xml> <output.xml> <location> <annotation>\n");
	fprintf(where, "  <location> may be:\n");
	fprintf(where, "    all, root, <type>:<logicalindex>, <type>:all\n");
	fprintf(where, "  <annotation> may be:\n");
	fprintf(where, "    info <name> <value>\n");
	fprintf(where, "    misc <name>\n");
	fprintf(where, "    distances <filename> [<flags>]\n");
	fprintf(where, "    memattr <name> <flags>\n");
	fprintf(where, "    memattr <name> <initiator> <value>\n");
        fprintf(where, "    cpukind <cpuset> <efficiency> <flags> [<infoname> <infovalue>]\n");
	fprintf(where, "    none\n");
        fprintf(where, "Options:\n");
	fprintf(where, "  --ci\tClear existing infos\n");
	fprintf(where, "  --ri\tReplace or remove existing infos with same name (annotation must be info)\n");
	fprintf(where, "  --cu\tClear existing userdata\n");
	fprintf(where, "  --cd\tClear existing distances\n");
}

static char *infoname = NULL, *infovalue = NULL;
static char *miscname = NULL;
static char *distancesfilename = NULL;

static char *maname = NULL;
static unsigned long maflags;

static char *mavname = NULL;
static hwloc_memattr_id_t mavid;
static hwloc_cpuset_t mavicpuset = NULL;
static char *maviobjstr = NULL;
static hwloc_obj_t maviobj = NULL;
static hwloc_uint64_t mavvalue;

static unsigned long distancesflags = 0;

static hwloc_cpuset_t ckcpuset = NULL;
static int ckefficiency = -1;
static unsigned long ckflags = 0;
static char * ckiname = NULL;
static char * ckivalue = NULL;

static int clearinfos = 0;
static int replaceinfos = 0;
static int clearuserdata = 0;
static int cleardistances = 0;

static void apply(hwloc_topology_t topology, hwloc_obj_t obj)
{
	unsigned i,j;
	if (clearinfos) {
		/* this may be considered dangerous, applications should not modify objects directly */
		for(i=0; i<obj->infos_count; i++) {
			struct hwloc_info_s *info = &obj->infos[i];
			free(info->name);
			free(info->value);
		}
		free(obj->infos);
		obj->infos = NULL;
		obj->infos_count = 0;
	}
	if (clearuserdata) {
		hwloc_utils_userdata_free(obj);
	}
	if (infoname) {
		if (replaceinfos) {
			/* this may be considered dangerous, applications should not modify objects directly */
			for(i=0, j=0; i<obj->infos_count; i++) {
				struct hwloc_info_s *info = &obj->infos[i];
				if (!strcmp(infoname, info->name)) {
					/* remove info */
					free(info->name);
					info->name = NULL;
					free(info->value);
				} else {
					if (i != j) {
						/* shift info to where it belongs */
						obj->infos[j].name = info->name;
						obj->infos[j].value = info->value;
					}
					j++;
				}
			}
			obj->infos_count = j;
			if (!j) {
				free(obj->infos);
				obj->infos = NULL;
			}
		}
		if (infovalue)
			hwloc_obj_add_info(obj, infoname, infovalue);
	}
	if (miscname)
		hwloc_topology_insert_misc_object(topology, obj, miscname);
        if (mavname) {
          struct hwloc_location loc, *locp = NULL;
          if (maviobj) {
            loc.type = HWLOC_LOCATION_TYPE_OBJECT;
            loc.location.object = maviobj;
            locp = &loc;
          } else if (mavicpuset) {
            loc.type = HWLOC_LOCATION_TYPE_CPUSET;
            loc.location.cpuset = mavicpuset;
            locp = &loc;
          }
          if (hwloc_memattr_set_value(topology, mavid, obj, locp, 0, mavvalue) < 0) {
            fprintf(stderr, "Failed to add memattr value (%s)\n", strerror(errno));
          }
        }
}

static void apply_recursive(hwloc_topology_t topology, hwloc_obj_t obj)
{
	hwloc_obj_t child = NULL;
	while ((child = hwloc_get_next_child(topology, obj, child)) != NULL)
		apply_recursive(topology, child);
	apply(topology, obj);
}

static void
hwloc_calc_process_location_annotate_cb(struct hwloc_calc_location_context_s *lcontext,
					void *_data __hwloc_attribute_unused,
					hwloc_obj_t obj)
{
	apply(lcontext->topology, obj);
}

static void
hwloc_calc_get_unique_obj_cb(struct hwloc_calc_location_context_s *lcontext __hwloc_attribute_unused,
			     void *_data,
			     hwloc_obj_t obj)
{
  hwloc_obj_t *objp = _data;
  if (!*objp)
    *objp = obj;
}

static hwloc_obj_t
get_unique_obj(hwloc_topology_t topology, int topodepth, char *str,
	       int *ignored_multiple)
{
  struct hwloc_calc_location_context_s lcontext;
  hwloc_obj_t obj;
  size_t length;
  size_t typelen;
  int err;

  typelen = strspn(str, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
  if (!typelen || str[typelen] != ':')
    return NULL;

  lcontext.topology = topology;
  lcontext.topodepth = topodepth;
  lcontext.only_hbm = -1;
  lcontext.logical = 1;
  lcontext.verbose = 0;
  obj = NULL;
  length = strspn(str+typelen+1, "0123456789");
  if (str[typelen+1+length] == '-' || str[typelen+1+length] == ':') {
    *ignored_multiple = 1;
  } else {
    *ignored_multiple = 0;
  }
  str[typelen+1+length] = '\0';
  err = hwloc_calc_process_location(&lcontext, str, typelen,
				    hwloc_calc_get_unique_obj_cb, &obj);
  if (err < 0)
    return NULL;
  return obj;
}


static void
add_distances(hwloc_topology_t topology, int topodepth)
{
	unsigned long kind = 0;
	unsigned nbobjs = 0;
	hwloc_obj_t *objs = NULL;
	hwloc_uint64_t *values = NULL;
	FILE *file;
	char line[64];
	unsigned i, x, y, z;
	int err;

	file = fopen(distancesfilename, "r");
	if (!file) {
		fprintf(stderr, "Failed to open distances file %s\n", distancesfilename);
		return;
	}

	if (!fgets(line, sizeof(line), file)) {
		fprintf(stderr, "Failed to read kind line\n");
		goto out;
	}
	kind = strtoul(line, NULL, 0);

	if (!fgets(line, sizeof(line), file)) {
		fprintf(stderr, "Failed to read nbobjs line\n");
		goto out;
	}
	nbobjs = strtoul(line, NULL, 0);
	if (nbobjs < 2) {
		fprintf(stderr, "Invalid distances with nbobjs == %u\n", nbobjs);
		goto out;
	}

	objs = malloc(nbobjs * sizeof(*objs));
	values = malloc(nbobjs*nbobjs * sizeof(*values));
	if (!objs || !values)
		goto out;

	for(i=0; i<nbobjs; i++) {
		hwloc_obj_t obj;
		int ignored_multiple;

		if (!fgets(line, sizeof(line), file)) {
		  fprintf(stderr, "Failed to read object line #%u.\n", i);
		  goto out;
		}

		obj = get_unique_obj(topology, topodepth, line, &ignored_multiple);
		if (!obj) {
		  fprintf(stderr, "Couldn't parse object line #%u.\n", i);
		  goto out;
		}

		if (ignored_multiple) {
		  fprintf(stderr, "Only the first object specified on line #%u was used.\n", i);
		}

		objs[i] = obj;
	}

	/* scan the first value line to see if we have all of them or just a combination */
	if (!fgets(line, sizeof(line), file)) {
		fprintf(stderr, "Failed to read object #%u line\n", i);
		goto out;
	}
	z = 1; /* default if sscanf finds only 2 values below */
	if (sscanf(line, "%u*%u*%u", &x, &y, &z) >= 2) {
		/* combination: generate the matrix to create x groups of y elements */
		unsigned j;
		if (x*y*z != nbobjs) {
			fprintf(stderr, "Invalid distances combination (%u*%u*%u=%u instead of %u)\n",
				x, y, z, x*y*z, nbobjs);
			goto out;
		}
		for(i=0; i<nbobjs; i++)
			for(j=0; j<nbobjs; j++)
				if (i==j)
					values[i*nbobjs+j] = 10;
				else if (i/z == j/z)
					values[i*nbobjs+j] = 20;
				else if (i/z/y == j/z/y)
					values[i*nbobjs+j] = 40;
				else
					values[i*nbobjs+j] = 80;
	} else {
		/* read all other values */
	  values[0] = strtoull(line, NULL, 10);

		for(i=1; i<nbobjs*nbobjs; i++) {
			if (!fgets(line, sizeof(line), file)) {
				fprintf(stderr, "Failed to read object #%u line\n", i);
				goto out;
			}
			values[i] = strtoull(line, NULL, 10);
		}
	}

	err = hwloc_distances_add(topology, nbobjs, objs, values, kind, distancesflags);
	if (err < 0) {
		fprintf(stderr, "Failed to add distances\n");
		goto out;
	}

out:
	free(objs);
	free(values);
	fclose(file);
	return;
}

int main(int argc, char *argv[])
{
	hwloc_topology_t topology;
	char *callname, *input, *output;
	char **locations;
	int nr_locations;
	int topodepth;
	int err;

	callname = argv[0];
	/* skip argv[0], handle options */
	argc--;
	argv++;

	hwloc_utils_check_api_version(callname);

	if (!getenv("HWLOC_XML_VERBOSE"))
		putenv((char *) "HWLOC_XML_VERBOSE=1");

	while (argc && *argv[0] == '-') {
		if (!strcmp(argv[0], "--ci"))
			clearinfos = 1;
		else if (!strcmp(argv[0], "--ri"))
			replaceinfos = 1;
		else if (!strcmp(argv[0], "--cu"))
			clearuserdata = 1;
		else if (!strcmp(argv[0], "--cd"))
			cleardistances = 1;
		else if (!strcmp(argv[0], "-h") || !strcmp(argv[0], "--help")) {
			usage(callname, stdout);
			exit(EXIT_SUCCESS);
		} else {
			fprintf(stderr, "Unrecognized options: %s\n", argv[0]);
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		argc--;
		argv++;
	}

	if (argc < 3) {
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}
	input = argv[0];
	output = argv[1];
	argc -= 2;
	argv += 2;

	if (!strcmp(argv[0], "--")) {
	  /* modern syntax with locations between "--" */
	  argc--;
	  argv++;
	  locations = &argv[0];
	  nr_locations = 0;
	  while (nr_locations < argc && strcmp(argv[nr_locations], "--"))
	    nr_locations++;
	  /* check we have an ending "--" */
	  if (nr_locations == argc || strcmp(argv[nr_locations], "--")) {
	    usage(callname, stderr);
	    exit(EXIT_FAILURE);
	  }
	  /* skip those locations and the ending "--" */
	  argc -= nr_locations+1;
	  argv += nr_locations+1;
	} else {
	  /* old syntax with a single location without "--" before and after */
	  locations = &argv[0];
	  nr_locations = 1;
	  argc--;
	  argv++;
	}

	if (argc < 1) {
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}
	if (!strcmp(argv[0], "info")) {
		if (argc < 2 || (!replaceinfos && argc < 3)) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		infoname = argv[1];
		infovalue = argc >= 3 ? argv[2] : NULL;

	} else if (!strcmp(argv[0], "misc")) {
		if (argc < 2) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		miscname = argv[1];

	} else if (!strcmp(argv[0], "distances")) {
		if (argc < 2) {
			usage(callname, stderr);
			exit(EXIT_FAILURE);
		}
		distancesfilename = argv[1];
		if (argc >= 3) {
			distancesflags = hwloc_utils_parse_distances_add_flags(argv[2]);
                        if(distancesflags == (unsigned long)-1)
                                goto out;
                }

        } else if (!strcmp(argv[0], "memattr")) {
                if (argc < 3) {
                        usage(callname, stderr);
                        exit(EXIT_FAILURE);
                }
                if (argc == 3) {
                        maname = argv[1];
                        maflags = hwloc_utils_parse_memattr_flags(argv[2]);
                } else {
                        mavname = argv[1];
                        mavvalue = strtoull(argv[3], NULL, 0);
                        if (strcmp(argv[2], "none")) {
                          if (!strncmp(argv[2], "0x", 2)) {
                            /* parse a cpuset */
                            mavicpuset = hwloc_bitmap_alloc();
                            if (!mavicpuset) {
                              fprintf(stderr, "Failed to allocate cpuset for memattr initiator\n");
                              goto out;
                            }
                            hwloc_bitmap_sscanf(mavicpuset, argv[2]);
                          } else {
                            /* parse an object */
                            maviobjstr = argv[2];
                          }
                        }
                }

        } else if (!strcmp(argv[0], "cpukind")) {
          if (argc < 4) {
            usage(callname, stderr);
            exit(EXIT_FAILURE);
          }
          ckcpuset = hwloc_bitmap_alloc();
          if (!ckcpuset) {
            fprintf(stderr, "Failed to allocate cpuset for cpukind\n");
            goto out;
          }
          hwloc_bitmap_sscanf(ckcpuset, argv[1]);
          ckefficiency = atoi(argv[2]);
          ckflags = atoi(argv[3]);
          if (argc == 5) {
            fprintf(stderr, "cpukind with info name without info value\n");
            goto out;
          }
          if (argc >= 6 && *argv[4] && *argv[5]) {
            ckiname = argv[4];
            ckivalue = argv[5];
          }

	} else if (!strcmp(argv[0], "none")) {
		/* do nothing (maybe clear) */
	} else {
		fprintf(stderr, "Unrecognized annotation type: %s\n", argv[0]);
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}

	if (replaceinfos && !infoname) {
		fprintf(stderr, "--ri missing a info name\n");
		usage(callname, stderr);
		exit(EXIT_FAILURE);
	}

	hwloc_topology_init(&topology);
	hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
	hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_INCLUDE_DISALLOWED | HWLOC_TOPOLOGY_FLAG_IMPORT_SUPPORT);
	err = hwloc_topology_set_xml(topology, input);
	if (err < 0)
		goto out_with_topology;

	putenv((char *) "HWLOC_XML_USERDATA_NOT_DECODED=1");
	hwloc_topology_set_userdata_import_callback(topology, hwloc_utils_userdata_import_cb);
	hwloc_topology_set_userdata_export_callback(topology, hwloc_utils_userdata_export_cb);

	err = hwloc_topology_load(topology);
	if (err < 0)
		goto out_with_topology;

	topodepth = hwloc_topology_get_depth(topology);

	if (cleardistances) {
		hwloc_distances_remove(topology);
	}

	if (distancesfilename) {
	  /* ignore locations */
	  add_distances(topology, topodepth);

        } else if (maname) {
          hwloc_memattr_id_t id;
          err = hwloc_memattr_register(topology, maname, maflags, &id);
          if (err < 0) {
            fprintf(stderr, "Failed to register new memattr (%s)\n", strerror(errno));
            goto out_with_topology;
          }

        } else if (ckcpuset) {
          struct hwloc_info_s info;
          info.name = ckiname;
          info.value = ckivalue;
          if (hwloc_cpukinds_register(topology, ckcpuset, ckefficiency,
                                      ckiname ? 1 : 0, ckiname ? &info : NULL,
                                      ckflags) < 0) {
            fprintf(stderr, "Failed to register CPU kind (%s)\n", strerror(errno));
          }

	} else {
	  int i;
          unsigned long mavflags = 0;

          if (mavname) {
            mavid = hwloc_utils_parse_memattr_name(topology, mavname);
            if (mavid == (hwloc_memattr_id_t) -1) {
              fprintf(stderr, "Failed to find memattr by name %s\n", mavname);
              goto out_with_topology;
            }
            hwloc_memattr_get_flags(topology, mavid, &mavflags);
          }

          if (maviobjstr && (mavflags & HWLOC_MEMATTR_FLAG_NEED_INITIATOR)) {
            int ignored_multiple;
            maviobj = get_unique_obj(topology, topodepth, maviobjstr, &ignored_multiple);
            if (!maviobj) {
              fprintf(stderr, "Failed to find memattr initiator object %s\n", maviobjstr);
              goto out_with_topology;
            }
            if (ignored_multiple) {
              fprintf(stderr, "Only the first object specified is used as a memattr initiator.\n");
            }
          }

	  for(i=0; i<nr_locations; i++) {
	    char *location = locations[i];
	    if (!strcmp(location, "all")) {
	      apply_recursive(topology, hwloc_get_root_obj(topology));
	    } else if (!strcmp(location, "root")) {
	      apply(topology, hwloc_get_root_obj(topology));
	    } else {
		size_t typelen;
		typelen = strspn(location, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
		if (typelen && (location[typelen] == ':' || location[typelen] == '=' || location[typelen] == '[')) {
			struct hwloc_calc_location_context_s lcontext;
			lcontext.topology = topology;
			lcontext.topodepth = topodepth;
			lcontext.only_hbm = -1;
			lcontext.logical = 1;
			lcontext.verbose = 0;
			err = hwloc_calc_process_location(&lcontext, location, typelen,
							  hwloc_calc_process_location_annotate_cb, NULL);
		}
	    }
	  }
	}

	err = hwloc_topology_export_xml(topology, output, 0);
	if (err < 0)
		goto out;

	hwloc_utils_userdata_free_recursive(hwloc_get_root_obj(topology));
	hwloc_topology_destroy(topology);
	exit(EXIT_SUCCESS);

out_with_topology:
	hwloc_utils_userdata_free_recursive(hwloc_get_root_obj(topology));
	hwloc_topology_destroy(topology);
out:
	hwloc_bitmap_free(mavicpuset);
        hwloc_bitmap_free(ckcpuset);
	exit(EXIT_FAILURE);
}
