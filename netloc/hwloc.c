/*
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */

#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <sys/types.h>
#include <dirent.h>

#include <private/netloc.h>
#include <netloc.h>
#include <hwloc.h>

int hwloc_get_core_number(int *pnum_cores)
{
    // TODO
    int num_cores;
    num_cores = 32;

    *pnum_cores = num_cores;
    return NETLOC_SUCCESS;
}

int netloc_read_hwloc(netloc_topology_t topology)
{
    int ret = 0;
    int err;

    char *hwloc_path;
    asprintf(&hwloc_path, "%s/../hwloc", topology->network->data_uri+7);

    UT_array *hwloc_topo_names;
    utarray_new(hwloc_topo_names, &ut_str_icd);
    UT_array *hwloc_topos;
    utarray_new(hwloc_topos, &ut_ptr_icd);

    DIR* dir = opendir(hwloc_path);
    /* Directory does not exist */
    if (!dir) {
        printf("Directory (%s) to hwloc does not exist\n", hwloc_path);
        return -1;
    }
    else {
        closedir(dir);
    }

    int num_diffs = 0;
    netloc_node_t *node, *node_tmp;
    netloc_topology_iter_nodes(topology, node, node_tmp) {
        char *hwloc_file;
        char *refname;

        /* We try to find a diff file */
        asprintf(&hwloc_file, "%s/%s.diff.xml", hwloc_path, node->hostname);
        hwloc_topology_diff_t diff;
        if ((err = hwloc_topology_diff_load_xml(hwloc_file, &diff, &refname)) >= 0) {
            refname[strlen(refname)-4] = '\0';
            hwloc_topology_diff_destroy(diff);
            num_diffs++;
        }
        else {
            free(hwloc_file);
            /* We try to find a regular file */
            asprintf(&hwloc_file, "%s/%s.xml", hwloc_path, node->hostname);
            FILE *fxml;
            if (!(fxml = fopen(hwloc_file, "r"))) {
                printf("Hwloc file absent: %s\n", hwloc_file);
            }
            else
                fclose(fxml);
            asprintf(&refname, "%s", node->hostname);
        }
        free(hwloc_file);

        /* Add the hwloc topology */
        int t = 0;
        while (t < utarray_len(hwloc_topo_names) &&
                strcmp(*(char **)utarray_eltptr(hwloc_topo_names, t), refname)) {
            t++;
        }
        /* Topology not found */
        if (t == utarray_len(hwloc_topo_names)) {
            utarray_push_back(hwloc_topo_names, &refname);

            /* Read the hwloc topology */
            hwloc_topology_t topology;
            hwloc_topology_init(&topology);

            ret = hwloc_topology_set_xml(topology, hwloc_path);
            if (ret == -1) {
                fprintf(stderr, "hwloc_topology_set_xml failed\n");
                return NETLOC_ERROR;
            }

            ret = hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
            if (ret == -1) {
                fprintf(stderr, "hwloc_topology_set_all_types_filter failed\n");
                return NETLOC_ERROR;
            }

            ret = hwloc_topology_load(topology);
            if (ret == -1) {
                fprintf(stderr, "hwloc_topology_load failed\n");
                return NETLOC_ERROR;
            }
            utarray_push_back(hwloc_topos, &topology);
        }
        /* Topology already present */
        else {
            free(refname);
        }
        node->hwlocTopoIdx = t;
    }

    if (!num_diffs) {
        printf("Warning: no hwloc diff file found!\n");
    }

    topology->topos = hwloc_topo_names;
    topology->hwloc_topos = (hwloc_topology_t *)hwloc_topos->d;

    printf("%d hwloc topologies found:\n", utarray_len(topology->topos));
    // XXX XXX XXX XXX XXX XXX XXX faire pareil pour recherche partitions
    for (int p = 0; p < utarray_len(topology->topos); p++) {
        printf("\t'%s'\n", *(char **)utarray_eltptr(topology->topos, p));
    }

    return ret;
}

int hwloc_to_netloc_arch(hwloc_topology_t topology, netloc_arch_t *arch)
{
    hwloc_obj_t root = hwloc_get_root_obj(topology);

    int depth = hwloc_topology_get_depth(topology);
    hwloc_obj_t first_object = root->first_child;

    UT_array **down_degrees_by_level;
    int *max_down_degrees_by_level;

    down_degrees_by_level = (UT_array **)malloc(depth*sizeof(UT_array *));
    for (int l = 0; l < depth; l++) {
        utarray_new(down_degrees_by_level[l], &ut_int_icd);
    }
    max_down_degrees_by_level = (int *)calloc(depth-1, sizeof(int));

    int level = depth-1;
    while (level <= 0) {
        int degree = 1;
        hwloc_obj_t current_object = first_object;
        // we go through the siblings
        while (current_object) {
            current_object = current_object->next_sibling;
            degree++;
        }
        /* Add the degree to the list of degrees */
        utarray_push_back(down_degrees_by_level[level], &degree);
        max_down_degrees_by_level[depth-1-level] =
            max_down_degrees_by_level[depth-1-level] > degree ?
            max_down_degrees_by_level[depth-1-level]: degree;

        current_object = current_object->next_cousin;

        if (!current_object) {
            first_object = first_object->first_child;
            level--;
        }
    }

    /* List of PUs */
    UT_array *ordered_hosts;
    utarray_new(ordered_hosts, &ut_int_icd);
    hwloc_obj_t current_object = first_object;
    while (current_object) {
        utarray_push_back(ordered_hosts, &first_object->os_index);
        current_object = current_object->next_cousin;
    }

    /* Weight for the edges in the tree */
    int *weight_by_level = (int *)malloc((depth)*sizeof(int));
    for (int l = 0; l < depth; l++) {
        weight_by_level[l] = depth;
    }

    netloc_arch_tree_t *tree = (netloc_arch_tree_t *)
        malloc(sizeof(netloc_arch_tree_t));
    tree->num_levels = depth;
    tree->degrees = max_down_degrees_by_level;
    tree->throughput = weight_by_level;

    netloc_arch_host_t *numbered_hosts;
    netloc_complete_tree(tree, down_degrees_by_level, &numbered_hosts);

    arch->arch.tree = tree;
    arch->type = NETLOC_ARCH_TREE;
    arch->topology = NULL;
    arch->num_hosts = utarray_len(ordered_hosts);
    arch->idx_hosts = numbered_hosts;
    arch->hosts = (int *)ordered_hosts->d;

    hwloc_topology_destroy(topology);

    return NETLOC_SUCCESS;
}
