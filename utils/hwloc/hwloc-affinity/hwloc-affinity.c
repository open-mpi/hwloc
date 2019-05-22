/*******************************************************************************
 * Copyright 2019 UChicago Argonne, LLC.
 * Author: Nicolas Denoyelle
 * SPDX-License-Identifier: BSD-3-Clause
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "private/autogen/config.h"
#include "affinity.h"
#include "utils.h"
#ifdef HAVE_SYS_PTRACE_H
#include "bind.h"
#endif

#define SHORT_OPT_LEN 2
#define OPTION_LEN 55
#define DESC_LEN 120
#define MINI(a,b) ((a) < (b) ? (a) : (b))

int desc_len = DESC_LEN;

static void print_option(const char *opt_short,
			 const char *opt_long,
			 const char *arg,
			 const char *arg2,
			 char *desc)
{
        int len, n, desclen = strlen(desc);
	char * descptr = desc;
	char * str = malloc(desc_len);
	memset(str, 0, desc_len);
	str[0] = '\t'; len = 1;
	
	if(opt_short != NULL){
	        n = snprintf(str + len, OPTION_LEN - len, "%-*s, ", SHORT_OPT_LEN, opt_short);
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
	printf("%s\n", str);
	free(str);
}

static void usage(const char * argv0)
{
	printf("SYNOPSIS:\n");
	printf("\tOutput a list of processing units with a particular policy.\n");
	printf("\tThis policy can further be used to bind an application threads\n");
	printf("\tweather by exporting the runtime environment to the\n");
	printf("\toutput value or starting the application with this tool.\n");
	printf("\n");

	printf("COMMAND:\n");
	printf("\t%s OPTIONS\n", argv0);
#ifdef HAVE_SYS_PTRACE_H
	printf("\t%s OPTIONS -- prog <args>\n", argv0);
#endif
	printf("\n");
	
	printf("OPTIONS:\n");
	print_option("-h","--help", NULL, NULL,
		     "Display this help.");	
	print_option("-i","--input","xml_topology", NULL,
		     "Use a topology from an XML file.");
	print_option("-n","--num","integer", NULL,
		     "Output only this number of objects.");
	print_option("-l","--logical", NULL, NULL,
		     "Output logical index instead of os index.");
	print_option("-s","--separator", "string", NULL,
		     "Set the separator character between indexes in output.");
	print_option(NULL,"--shuffle", NULL, NULL,
		     "Randomize policy indexes. Only with tleaf policy.");
	print_option("-r","--restrict", "cpuset OR hwloc_obj:logical_index", NULL,
		     "Restrict topology to cpuset or a topology object cpuset/nodeset.");
	print_option("-p", "--policy", "policy", "policy_arg",
		     "Set enumeration policy. Policy can be one of: scatter, round-robin, tleaf. See POLICIES below for further informations on policies and their arguments.");
	printf("\n");
	
	printf("POLICIES:\n");
	print_option(NULL,
		     "round-robin",
		     "topology level",
		     NULL,
		     "Output one Processing Unit (PU) per topology object of level iterated in a round-robin fashion. If oversubscribing happens, i.e objects are queried more than once, then PU below topology level will be iterated, also in a round-robin fashion. This policy is used to favor neighbors locality");
	print_option(NULL,
		     "scatter",
		     "topology level",
		     NULL,
		     "Output Processing Unit (PU) of the topology object enumerated in a scatter fashion, i.e object are enumerated such that next PU is as far as possible to previous PU indexes. If oversubscribing happens, i.e objects are queried more than once, then PU below topology level will be iterated in a round-robin fashion. This policy is known to favor balance and exclusivity of hierarchical topology resources.");
	print_option(NULL,
		     "tleaf",
		     "topology_level:topology_level:...",
		     NULL,
		     "tleaf policy is a user customized policy. A tleaf is a tree where all nodes of a level has the same arity. Such property is verified in most machines topology. It is still possible to map a tleaf on a topology which does not exhibit such property by expanding the tleaf to span the topology and ignore unachievable nodes. tleaf policy is an iterator on tleaf leaves, i.e the deepest topology object it contains. Leaves coordinates of a tleaf are as numerous as their is level in the tleaf, and their range of value is an index of tleaf node level arity. The way these coordinates are incremented sets the policy on which leaves are iterated. These coordinates iteration is done from the first provided topology level to the last topology level. Round robin-policy is typically a tleaf where levels are stacked from the processing units to the topology root and scatter policy is a tleaf where levels are stacked in the opposite order. In the round-robin policy, leaves of a parent node are iterated. Then next parent is looked for and its leaves are iterated, and so on.");
}

// All options have valid default values.
char * topology_input = NULL;
int    num_index      = 0;
int    logical_output = 0;
char * separator      = " ";
int    shuffle        = 0;
char * restrict_topo  = NULL;
char * policy         = "round-robin";
char * policy_arg     = "Core";
#ifdef HAVE_SYS_PTRACE_H
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
	if(strlen(argv[i]) != strlen(short_opt) && strlen(argv[i]) != strlen(long_opt))
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
			usage(argv[0]);
			exit(1);
		}
		else if(match_opt(i, argc, argv, "-i", "--input", 1))
			topology_input = argv[++i];
		else if(match_opt(i, argc, argv, "-n", "--num", 1))
			num_index = atoi(argv[++i]);
		else if(match_opt(i, argc, argv, "-l", "--logical", 0))
			logical_output = 1;
		else if(match_opt(i, argc, argv, "-s", "--separator", 1))
			separator = argv[++i];
		else if(match_opt(i, argc, argv, "NO_OPT", "--shuffle", 0))
			shuffle = 1;
		else if(match_opt(i, argc, argv, "-r", "--restrict", 1))
			restrict_topo = argv[++i];
		else if(match_opt(i, argc, argv, "-p", "--policy", 2)){
			policy = argv[++i];
			policy_arg = argv[++i];
		}
#ifdef HAVE_SYS_PTRACE_H		
		else if(match_opt(i, argc, argv, "NO_OPT", "--", 1)){
			user_argv = &(argv[++i]);
			break;
		}
#endif
		else {
			fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
			exit(1);
		}
	}
}

static int restrict_topology(hwloc_topology_t topology)
{
	int hwloc_out = 0, err = 0;
	hwloc_obj_t restrict_obj;
	hwloc_bitmap_t restrict_cpuset;

	restrict_cpuset = hwloc_bitmap_alloc();	
	if(restrict_cpuset == NULL){
		perror("hwloc_bitmap_alloc");
		return -1;
	}	

	hwloc_out = hwloc_bitmap_sscanf(restrict_cpuset, restrict_topo);
	restrict_obj = hwloc_obj_from_string(topology, restrict_topo);
			
	if(restrict_obj != NULL){
		err = hwloc_topology_restrict_obj(topology, restrict_obj);
		if(err < 0){
			perror("hwloc_topology_restrict");
		}
	}
	else if(hwloc_out >= 0) {
		err = hwloc_topology_restrict(topology, restrict_cpuset,
					      HWLOC_RESTRICT_FLAG_REMOVE_CPULESS);
		if(err < 0){
			perror("hwloc_topology_restrict");
		}
	}
	else {
		err = -1;
		fprintf(stderr, "Restrict string %s could not be interpreted as a valid hwloc_obj:logical_index or hwloc_cpuset_t\n", restrict_topo);
	}
	
	hwloc_bitmap_free(restrict_cpuset);
	return err;			
}

static int build_policy(hwloc_topology_t topology,
			struct cpuaffinity_enum **out)
{
	int err = 0;
	hwloc_obj_type_t level;
	int round_robin = !strcmp(policy, "round-robin");
	int scatter = !strcmp(policy, "scatter");
	int tleaf = !strcmp(policy, "tleaf");
	
	if(round_robin || scatter){
		err = hwloc_type_sscanf(policy_arg, &level, NULL, 0);
		if(err < 0){
			fprintf(stderr,
				"Level %s of policy %s is not a valid hwloc obj type.\n",
				policy_arg,
				policy);
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
		int depth = hwloc_topology_get_depth(topology);
		int n_levels = 0;
		hwloc_obj_type_t *levels;
		char *level_str;
		char *levels_str;
		char *save_ptr;
		
		levels = malloc(depth * sizeof(*levels));
		if(levels == NULL){
			perror("malloc");
			err = -1;
			goto out_policy;			       
		}
		levels_str = strdup(policy_arg);
		if(levels_str == NULL){
			perror("strdup");
			err = -1;
			goto out_with_levels;			       
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
						&levels[n_levels++],
						NULL, 0);
			if(err < 0){
				fprintf(stderr,
					"Level %s is not a valid hwloc obj\n",
					level_str);
				goto out_with_save_ptr;
			}
			level_str = strtok(NULL, ":");			
		}

		if(n_levels < 2){
			err = -1;
			fprintf(stderr, "tleaf policy requires at least two topology levels.\n");
			goto out_with_save_ptr;
		}
		*out = cpuaffinity_tleaf(topology, n_levels, levels, shuffle);
		if(*out == NULL){
			err = -1;
			if(errno == EINVAL)
				fprintf(stderr, "All provided topology levels must have a positive depth.\n");
			else
				perror("cpuaffinity_tleaf");
		}
		
	out_with_save_ptr:
		free(save_ptr);
	out_with_levels:
		free(levels);
	}
	else {
		err = -1;
		fprintf(stderr, "Policy %s is not a valid policy.\nValid policies are: round-robin, scatter, tleaf\n", policy);
	}
 out_policy:
	return err;
}
	

int main(int argc, char **argv){
	int err = 0;
	hwloc_topology_t topology;
	struct cpuaffinity_enum *affinity;

	srand(time(NULL));
	parse_options(argc, argv);

	// Build topology
	topology = hwloc_affinity_topology_load(topology_input);
	if(topology == NULL){
		perror("topology_load");
		return 1;
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
#ifdef HAVE_SYS_PTRACE_H	
	if(user_argv == NULL)
#endif
		cpuaffinity_enum_print(affinity,
				       separator,
				       logical_output,
				       num_index);
	
#ifdef HAVE_SYS_PTRACE_H
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

#ifdef HAVE_SYS_PTRACE_H	
 out_with_affinity:
#endif
	cpuaffinity_enum_free(affinity);	
 out_with_topology:
	hwloc_topology_destroy(topology);
	
	return err;
}

