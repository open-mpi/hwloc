/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
 * See COPYING in top-level directory.
 ****************************************************************************/

#include "misc.h"
#include "hwloc-calc.h"
#include "hwloc-thread-bind.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <unistd.h>

//Options with default values.
static char * topology_input = NULL;
static char * input_format   = NULL;
static char * restrict_topo  = NULL;
static int    round_robin    = 0;
static char **locations_str  = NULL;
static char **user_argv      = NULL;
int logical = 0; // Whether indexing is logical
int verbose = 0; // Whether to verbose print. 

// Structure used for accessing threads binding locations.
static hwloc_topology_t topology;
static struct cpuaffinity_enum *binding;

static void hwloc_process_locations(void)
{
	char **loc_str;
	hwloc_cpuset_t location_cpuset;
	hwloc_obj_t location;
	for(loc_str = locations_str; loc_str < user_argv-1; loc_str++){
		location_cpuset = hwloc_process_location(topology, *loc_str);
		if(location_cpuset == NULL)
			exit(1);
		location = hwloc_get_first_largest_obj_inside_cpuset(topology,
								     location_cpuset);
		hwloc_bitmap_free(location_cpuset);
		if(location == NULL)
			exit(1);
		if(cpuaffinity_enum_append(binding, location) != 0)
			exit(1);
	}
}

/***************************************************************************/
/*                                   Main                                  */
/***************************************************************************/

void usage(const char *callname __hwloc_attribute_unused, FILE *where)
{
	fprintf(where, "hwloc-thread-bind is a utility to bind threads in the order of their creation to specific cpusets.\n\n");

	fprintf(where, "Usage: hwloc-thread-bind [options] <location> ... -- <command-line>\n");
	fprintf(where, " <location> may be a space-separated list of cpusets or objects\n");
	fprintf(where, "            as supported by the hwloc-bind utility, e.g:\n");
	hwloc_calc_locations_usage(where);

	fprintf(where, "Binding options:\n");
	fprintf(where, "  -r --round-robin          If more threads are created than locations provided, then round-robin on locations to bind extra threads instead of not binding them.\n");

	fprintf(where, "Formatting options:\n");
	fprintf(where, "  -l --logical              Use logical object indexes (default)\n");
	fprintf(where, "  -p --physical             Use physical object indexes\n");
	fprintf(where, "  --restrict <cpuset>       Restrict the topology to processors listed in <cpuset>\n");	
	hwloc_utils_input_format_usage(where, 10);
	
	fprintf(where, "Miscellaneous options:\n");
	fprintf(where, "  -v --verbose              Show verbose messages\n");
	fprintf(where, "  --version                 Report version and exit\n");
}

static int match_opt(const int    i,
		     int    argc,
		     char **argv,
		     const char  *short_opt,
		     const char  *long_opt,
		     const int    num_args)
{
	if(i > argc){
		fprintf(stderr, "Option %s does not require argument\n", argv[i]);
	        exit(1);
	}
	
	if(strcmp(argv[i], short_opt) && strcmp(argv[i],long_opt))
		return 0;
	
	if(strlen(argv[i]) != strlen(short_opt) &&
	   strlen(argv[i]) != strlen(long_opt))
		return 0;
	
	if(i+num_args >= argc){
		fprintf(stderr, "Option %s requires %d %s.\n",
			argv[i],
			num_args,
			num_args > 1 ? "arguments" : "argument");
		exit(1);
	}
	
	return 1;
}

static void parse_options(int argc, char **argv)
{
	int i = 0;
	while(++i < argc){
		if(match_opt(i, argc, argv, "-h", "--help", 0)){
			usage(argv[0], stdout);
			exit(1);
		}
		else if(match_opt(i, argc, argv, "!Do not match short option!", "--version", 0)){
			printf("%s %s\n", argv[0], HWLOC_VERSION);
			exit(EXIT_SUCCESS);
		}
		else if(match_opt(i, argc, argv, "-l", "--logical", 0))
			logical = 1;
		else if(match_opt(i, argc, argv, "-p", "--physical", 0))
			logical = 0;
		else if(match_opt(i, argc, argv, "-v", "--verbose", 0))
			verbose = 1;
		else if(match_opt(i, argc, argv, "-r", "--round-robin", 0))
			round_robin = 1;
		else if(match_opt(i, argc, argv, "!Do not match short option!", "--restrict", 1))
			restrict_topo = argv[++i];
		else if(match_opt(i, argc, argv, "!Do not match short option!", "--", 1)){
			user_argv = &(argv[++i]);
			break;
		}
		else if (locations_str == NULL)
			locations_str = &(argv[i]);
	}

	// Check that required options are provided.
	if(user_argv == NULL){
		fprintf(stderr, "Must provide a command line to run.\n");
		exit(1);
	}
	if(locations_str == NULL){
		fprintf(stderr, "Must provide a list of cpuset where to bind threads.\n");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int err = 0;
	enum hwloc_utils_input_format format;
	pid_t pid;
	
	hwloc_utils_check_api_version(argv[0]);
	parse_options(argc, argv);
	
	// Build topology
	if (hwloc_topology_init(&topology)) {
		perror("hwloc_topology_init");
		return -1;
	}

	if(topology_input != NULL){
		format = input_format != NULL ?
			hwloc_utils_parse_input_format(input_format,
						       argv[0]) :
			hwloc_utils_autodetect_input_format(topology_input, 0);
		if (hwloc_utils_enable_input_format(topology,
																				HWLOC_TOPOLOGY_FLAG_IS_THISSYSTEM,
																				topology_input,
																				&format, 0,
																				argv[0]) != EXIT_SUCCESS)
			{
				err = -1;
				goto out_with_topology;
			}
	}

	if (hwloc_topology_load(topology) != 0) {
		perror("hwloc_topology_load");
	        err = -1;
	        goto out_with_topology;
	}

	// Restrict topology
	if(restrict_topo != NULL && restrict_topology(topology, restrict_topo) < 0){
		err = -1;
		goto out_with_topology;
	}


	// Parse locations
	binding = cpuaffinity_enum_alloc(topology);
	if(binding == NULL){
		err = -1;
		goto out_with_topology;
	}
	hwloc_process_locations();
	
	// Bind program threads
	pid = fork();  

	/* Tracee */
	if(pid == 0) {
		// Stop child itself, it will be resumed by cpuaffinity_attach() or
		// killed if the call fails.
		kill(getpid(), SIGSTOP);
		// Start command when parent resume this child.
		if(execvp(user_argv[0], user_argv) == -1){
			perror("execvp");
			return -1;
		}
		return 0;
	}

	/* Tracer code */
	else if(pid > 0) {
		// Attach
	        err = cpuaffinity_attach(pid, binding, round_robin, 1);
		if(err == -1){
			// Could not attach to child. Kill it then.
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
		} else {
			wait(&err);
			if(WIFEXITED(err))
			        err = WEXITSTATUS(err);
		}
	}

	/* Fork error */
	else {
		perror("fork");
	        err = -1;
	}

	cpuaffinity_enum_free(binding);
 out_with_topology:
	hwloc_topology_destroy(topology);
	
	return err;
}
