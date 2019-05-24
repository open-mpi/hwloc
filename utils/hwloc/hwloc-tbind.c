/***************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
****************************************************************************/

#include "private/autogen/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "hwloc-calc.h"
#include "hwloc-tbind.h"
#include "misc.h"

/***************************************************************************/
/*                                   Main                                  */
/***************************************************************************/

#define SHORT_OPT_LEN 2
#define OPTION_LEN 55
#define DESC_LEN 120
#define MINI(a,b) ((a) < (b) ? (a) : (b))

int desc_len = DESC_LEN;

static void print_option(const char *opt_short,
			 const char *opt_long,
			 const char *arg,
			 const char *arg2,
			 char *desc,
			 FILE *where)
{
        int len, n, desclen = strlen(desc);
	char * descptr = desc;
	char * str = malloc(desc_len);
	memset(str, 0, desc_len);
	str[0] = '\t'; len = 1;
	
	if(opt_short != NULL){
	        n = snprintf(str + len, OPTION_LEN - len,
			     "%-*s, ",
			     SHORT_OPT_LEN,
			     opt_short);
		len += MINI(n, OPTION_LEN - len -1);
	}

        n = snprintf(str + len, OPTION_LEN - len, "%s", opt_long);
	len += MINI(n, OPTION_LEN - len -1);
	
	if(arg != NULL){
		n = snprintf(str + len, OPTION_LEN - len, " <%s>", arg);
		len += MINI(n, OPTION_LEN - len -1);
	}		
	
	if(arg2 != NULL){
	        n = snprintf(str + len, OPTION_LEN - len, " <%s>", arg2);
		len += MINI(n, OPTION_LEN - len -1);
	}

	n = snprintf(str + len, OPTION_LEN - len, ":");
	len += MINI(n, OPTION_LEN - len -1);

	n = snprintf(str+len, OPTION_LEN-len+1, "%s",
		     "...............................................");
	len += MINI(n, OPTION_LEN - len -1);

	while(desc && (descptr - desc) < desclen){
		if(len >= desc_len-1){
			printf("%s\n", str);
			memset(str, 0, desc_len);
			str[0]='\t';
			str[1]='\t';
			len=2;
		}
		n = snprintf(str+len, desc_len - len, "%s", descptr);
		n = MINI(n, desc_len - len - 1);
		descptr += n;
		len += n;
	}
	fprintf(where, "%s\n", str);
	free(str);
}

void usage(const char * argv0, FILE *where)
{
	fprintf(where, "SYNOPSIS:\n");
	fprintf(where, "\tOutput a list of processing units with a particular policy.\n");
	fprintf(where, "\tThis policy can further be used to bind an application threads\n");
	fprintf(where, "\tweather by exporting the runtime environment to the\n");
	fprintf(where, "\toutput value or starting the application with this tool.\n");
	fprintf(where, "\n");

	fprintf(where, "COMMAND:\n");
	fprintf(where, "\t%s OPTIONS\n", argv0);
#ifdef HWLOC_HAVE_PTRACE
	fprintf(where, "\t%s OPTIONS -- prog <args>\n", argv0);
#endif
	fprintf(where, "\n");
	
	fprintf(where, "OPTIONS:\n");
	print_option("-h","--help", NULL, NULL,
		     "Display this help.", where);
	print_option("-v","--version", NULL, NULL,
		     "Print hwloc version and exit.", where);		
	print_option("-i","--input","<input>", NULL,
		     "Use a topology from an XML file, or any input supported by hwloc-calc(1).", where);
	print_option("--if", "--input-format","<input-format>", NULL,
		     "Enforce the input in the given format, among xml, fsroot, cpuid and synthetic.", where);	
	print_option("-n","--num","integer", NULL,
		     "Output only this number of objects.", where);
	print_option("-l","--logical", NULL, NULL,
		     "Output logical index instead of os index. If set, also input indexes to --restrict option will be considered as logical.", where);
	print_option("-c","--cpuset", NULL, NULL,
		     "Output topology objects cpuset instead of their indexes", where);	
	print_option("-t","--taskset", NULL, NULL,
		     "Output taskset format of topology objects cpuset instead of their indexes", where);
	print_option(NULL,"--reverse", NULL, NULL,
		     "Output objects in reverse order.", where);
	print_option("-s","--separator", "string", NULL,
		     "Set the separator character between indexes in output.", where);
	print_option(NULL,"--shuffle", NULL, NULL,
		     "Randomize policy indexes. Only with tleaf policy.", where);
	print_option("-r","--restrict", "cpuset OR hwloc_obj:logical_index", NULL,
		     "Restrict topology to cpuset or a topology object cpuset/nodeset.", where);
	print_option("-p", "--policy", "policy", "policy_arg",
		     "Set enumeration policy. Policy can be one of: scatter, round-robin, tleaf. See POLICIES below for further informations on policies and their arguments.", where);
	fprintf(where, "\n");
	
	fprintf(where, "POLICIES:\n");
	print_option(NULL,
		     "round-robin",
		     "topology level",
		     NULL,
		     "Output one Processing Unit (PU, where) per topology object of level iterated in a round-robin fashion. If oversubscribing happens, i.e objects are queried more than once, then PU below topology level will be iterated, also in a round-robin fashion. This policy is used to favor neighbors locality", where);
	print_option(NULL,
		     "scatter",
		     "topology level",
		     NULL,
		     "Output Processing Unit (PU, where) of the topology object enumerated in a scatter fashion, i.e object are enumerated such that next PU is as far as possible to previous PU indexes. If oversubscribing happens, i.e objects are queried more than once, then PU below topology level will be iterated in a round-robin fashion. This policy is known to favor balance and exclusivity of hierarchical topology resources.", where);
	print_option(NULL,
		     "tleaf",
		     "topology_level:topology_level:...",
		     NULL,
		     "tleaf policy is a user customized policy. A tleaf is a tree where all nodes of a level has the same arity. Such property is verified in most machines topology. It is still possible to map a tleaf on a topology which does not exhibit such property by expanding the tleaf to span the topology and ignore unachievable nodes. tleaf policy is an iterator on tleaf leaves, i.e the deepest topology object it contains. Leaves coordinates of a tleaf are as numerous as their is level in the tleaf, and their range of value is an index of tleaf node level arity. The way these coordinates are incremented sets the policy on which leaves are iterated. These coordinates iteration is done from the first provided topology level to the last topology level. Round robin-policy is typically a tleaf where levels are stacked from the processing units to the topology root and scatter policy is a tleaf where levels are stacked in the opposite order. In the round-robin policy, leaves of a parent node are iterated. Then next parent is looked for and its leaves are iterated, and so on.", where);
}

// All options have valid default values.
char * topology_input = NULL;
char * input_format   = NULL;
int    num_index      = 0;
int    logical_opt    = 0;
int    cpuset_opt     = 0;
int    taskset_opt    = 0;
char * separator      = " ";
int    shuffle        = 0;
int    reverse        = 0;
char * restrict_topo  = NULL;
char * policy         = "round-robin";
char * policy_arg     = "Core";
#ifdef HWLOC_HAVE_PTRACE
char **user_argv      = NULL;
#endif

static int match_opt(int i,
		     int argc,
		     char ** argv,
		     const char* short_opt,
		     const char * long_opt,
		     const int num_options)
{
	if(i > argc){
		fprintf(stderr, "Parsing to many argument\n");
	        exit(1);
	}
	if(strcmp(argv[i], short_opt) && strcmp(argv[i],long_opt))
		return 0;
	if(strlen(argv[i]) != strlen(short_opt) &&
	   strlen(argv[i]) != strlen(long_opt))
		return 0;
	
	if(i+num_options >= argc){
		fprintf(stderr, "Option %s requires %d %s.\n",
			argv[i],
			num_options,
			num_options > 1 ? "arguments" : "argument");
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
		else if(match_opt(i, argc, argv, "-v", "--version", 0)){
			printf("%s %s\n", argv[0], HWLOC_VERSION);
			exit(EXIT_SUCCESS);
		}
 		else if(match_opt(i, argc, argv, "-i", "--input", 1))
			topology_input = argv[++i];
 		else if(match_opt(i, argc, argv, "--if", "--input-format", 1))
		        input_format = argv[++i];		
		else if(match_opt(i, argc, argv, "-n", "--num", 1))
			num_index = atoi(argv[++i]);
		else if(match_opt(i, argc, argv, "-l", "--logical", 0))
			logical_opt = 1;
		else if(match_opt(i, argc, argv, "-c", "--cpuset", 0))
			cpuset_opt = 1;
		else if(match_opt(i, argc, argv, "-t", "--taskset", 0))
			taskset_opt = 1;
		else if(match_opt(i, argc, argv, "NO_OPT", "--reverse", 0))
		        reverse = 1;
		else if(match_opt(i, argc, argv, "NO_OPT", "--shuffle", 0))
			shuffle = 1;
		else if(match_opt(i, argc, argv, "-r", "--restrict", 1))
			restrict_topo = argv[++i];
		else if(match_opt(i, argc, argv, "-p", "--policy", 2)){
			policy = argv[++i];
			policy_arg = argv[++i];
		}
#ifdef HWLOC_HAVE_PTRACE		
		else if(match_opt(i, argc, argv, "NO_OPT", "--", 1)){
			user_argv = &(argv[++i]);
			break;
		}
#endif
		else {
			fprintf(stderr,
				"Unrecognized option: %s\n",
				argv[i]);
			exit(1);
		}
	}
	if(cpuset_opt  + taskset_opt + logical_opt > 1){
		fprintf(stderr, "--logical, --taskset and --cpuset options cannot be used together.");
		exit(1);
	}	
}

static int restrict_topology(hwloc_topology_t topology)
{
	int err = 0;
	hwloc_bitmap_t restrict_cpuset;
	
	restrict_cpuset = hwloc_bitmap_alloc();	
	if(restrict_cpuset == NULL){
		perror("hwloc_bitmap_alloc");
		return -1;
	}	
	
	struct hwloc_calc_location_context_s lcontext = {
		.topology = topology,
		.topodepth = hwloc_topology_get_depth(topology),
		.only_hbm = -1,
		.logical = logical_opt,
		.verbose = 0
	};
	struct hwloc_calc_set_context_s scontext = {
		.nodeset_input = 0,
		.nodeset_output = 0,
		.output_set = restrict_cpuset,
	};
	
	err = hwloc_calc_process_location_as_set(&lcontext,
						 &scontext,
						 restrict_topo);
	if(err != 0)
		goto out;

	err = hwloc_topology_restrict(topology,
				      restrict_cpuset,
				      HWLOC_RESTRICT_FLAG_REMOVE_CPULESS);
	if(err != 0)
		perror("hwloc_topology_restrict");

 out:
	hwloc_bitmap_free(restrict_cpuset);
	return err;			
}

static int build_policy(hwloc_topology_t topology,
			struct cpuaffinity_enum **out)
{
	int err = 0;
	hwloc_obj_type_t level;
	int depth;
	int round_robin = !strcmp(policy, "round-robin");
	int scatter = !strcmp(policy, "scatter");
	int tleaf = !strcmp(policy, "tleaf");
	
	if(round_robin || scatter){
		err = hwloc_type_sscanf(policy_arg,
					&level,
					NULL,
					0);
		if(err < 0){
			fprintf(stderr,
				"Level %s of policy %s is not a valid hwloc obj type.\n",
				policy_arg,
				policy);
			goto out_policy;
		}
		depth = hwloc_get_type_depth(topology, level);
		if(depth < 0){
			fprintf(stderr, "Invalid level: \"%s\". topology-level must be at a positive depth.\n",
				policy_arg);
			err = -1;
			goto out_policy;
		}
		if(round_robin)
			*out = cpuaffinity_round_robin(topology, level);
		else if(scatter)
			*out = cpuaffinity_scatter(topology, level);
		if(*out == NULL){
			err = -1;
			if(errno == EINVAL)
				fprintf(stderr,
					"Policy level must at a positive depth.\n");
			else
				perror("cpuaffinity_scatter/round_robin");
		}
	}
	else if(tleaf){
	        depth = hwloc_topology_get_depth(topology);
		int level_depth;
		int n_levels = 0;
		int  *depths;
		hwloc_obj_type_t type;
		char *level_str;
		char *levels_str;
		char *save_ptr;
		
		depths = malloc(depth * sizeof(*depths));
		if(depths == NULL){
			perror("malloc");
			err = -1;
			goto out_policy;			       
		}
		levels_str = strdup(policy_arg);
		if(levels_str == NULL){
			perror("strdup");
			err = -1;
			goto out_with_depths;			       
		}
		save_ptr = levels_str;

		level_str = strtok(levels_str, ":");
		while(level_str != NULL){
			if(n_levels >= depth){
				err = -1;
				fprintf(stderr,
					"There can't be more levels than topology depth.\n");
				goto out_with_save_ptr;
			}
				
			err = hwloc_type_sscanf(level_str,
						&type,
						NULL, 0);
			if(err < 0){
				fprintf(stderr,
					"Level %s is not a valid hwloc obj\n",
					level_str);
				goto out_with_save_ptr;
			}
			level_depth = hwloc_get_type_depth(topology, type);
			if(level_depth < 0){
				fprintf(stderr, "topology-level must be at a positive depth\n");
			        goto out_with_save_ptr;
			}
			depths[n_levels++] = level_depth;


			level_str = strtok(NULL, ":");			
		}

		if(n_levels < 2){
			err = -1;
			fprintf(stderr,
				"tleaf policy requires at least two topology levels.\n");
			goto out_with_save_ptr;
		}
		*out = cpuaffinity_tleaf(topology, n_levels, depths, shuffle);
		if(*out == NULL){
			err = -1;
			if(errno == EINVAL)
				fprintf(stderr,
					"All provided topology levels must have a positive depth.\n");
			else
				perror("cpuaffinity_tleaf");
		}
		
	out_with_save_ptr:
		free(save_ptr);
	out_with_depths:
		free(depths);
	}
	else {
		err = -1;
		fprintf(stderr,
			"Policy %s is not a valid policy.\nValid policies are: round-robin, scatter, tleaf\n",
			policy);
	}
 out_policy:
	return err;
}
	

int main(int argc, char **argv)
{
	int err = 0;
	hwloc_topology_t topology;
	struct cpuaffinity_enum *affinity;
	enum hwloc_utils_input_format format;
	
	hwloc_utils_check_api_version(argv[0]);
	srand(time(NULL));
	parse_options(argc, argv);

	// Build topology
	if (hwloc_topology_init(&topology)) {
		perror("hwloc_topology_init");
		return 1;
	}
	if(topology_input != NULL){
		format = input_format != NULL ?
			hwloc_utils_parse_input_format(input_format,
						       argv[0]) :
			hwloc_utils_autodetect_input_format(topology_input,
							    0);
		if(hwloc_utils_enable_input_format(topology,
						   topology_input,
						   &format,
						   0,
						   argv[0]) != EXIT_SUCCESS){
			err = 1;
			goto out_with_topology;
		}
	}
	if (hwloc_topology_load(topology) != 0) {
		perror("hwloc_topology_load");
	        err = 1;
	        goto out_with_topology;
	}

	// Restrict topology
	if(restrict_topo != NULL && restrict_topology(topology) < 0){
		err = 1;
		goto out_with_topology;
	}
		
	// Build policy
	if(build_policy(topology, &affinity) < 0){
		err = 1;
		goto out_with_topology;
	}
	
	// Ouput object index
#ifdef HWLOC_HAVE_PTRACE	
	if(user_argv == NULL)
#endif
		cpuaffinity_enum_print(affinity,
				       separator,
				       logical_opt,
				       cpuset_opt,
				       taskset_opt,
				       reverse,
				       num_index);
	
#ifdef HWLOC_HAVE_PTRACE
	// Bind program threads
        if(user_argv != NULL){
		pid_t child;
		
		child = cpuaffinity_exec(affinity, user_argv);
		if(child <= 0){
			err = child;
			goto out_with_affinity;
		}
		wait(&err);
		if(WIFEXITED(err))
			err = WEXITSTATUS(err);		
	}
#endif

#ifdef HWLOC_HAVE_PTRACE	
 out_with_affinity:
#endif
	cpuaffinity_enum_free(affinity);	
 out_with_topology:
	hwloc_topology_destroy(topology);
	
	return err;
}
