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

int netloc_read_hwloc(netloc_topology_t topology, int num_nodes, netloc_node_t **node_list)
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

    if (!num_nodes) {
        netloc_node_t *node, *node_tmp;
        num_nodes = HASH_COUNT(topology->nodes);
        node_list = (netloc_node_t **)malloc(sizeof(netloc_node_t *[num_nodes]));
        int n = 0;
        netloc_topology_iter_nodes(topology, node, node_tmp) {
            node_list[n++] = node;
        }
    }

    for (int n  = 0; n < num_nodes; n++) {
        netloc_node_t *node = node_list[n];
        char *hwloc_file;
        char *refname;

        if (netloc_node_is_switch(node))
            continue;

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
            hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);

            char *hwloc_ref_path;
            asprintf(&hwloc_ref_path, "%s/%s.xml", hwloc_path, refname);
            ret = hwloc_topology_set_xml(topology, hwloc_ref_path);
            free(hwloc_ref_path);
            if (ret == -1) {
                void *topology = NULL;
                utarray_push_back(hwloc_topos, &topology);
                fprintf(stdout, "Warning: no topology for %s\n", refname);
                continue;
            }

            ret = hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
            if (ret == -1) {
                fprintf(stderr, "hwloc_topology_set_all_types_filter failed\n");
                return NETLOC_ERROR;
            }

            ret = hwloc_topology_set_io_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_NONE);
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
        free(hwloc_file);
        node->hwlocTopo = *(hwloc_topology_t *)utarray_eltptr(hwloc_topos, t);
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

    return NETLOC_SUCCESS;
}

/* Set the info from hwloc of the node in the correspondig arch */
int hwloc_to_netloc_arch(netloc_arch_node_t *arch)
{
    hwloc_topology_t topology = arch->node->hwlocTopo;

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
    hwloc_obj_t current_object = first_object;
    while (level >= 1) {
        int degree = 1;
        /* we go through the siblings */
        while (current_object->next_sibling) {
            current_object = current_object->next_sibling;
            degree++;
        }
        /* Add the degree to the list of degrees */
        utarray_push_back(down_degrees_by_level[depth-1-level], &degree);
        max_down_degrees_by_level[depth-1-level] =
            max_down_degrees_by_level[depth-1-level] > degree ?
            max_down_degrees_by_level[depth-1-level] : degree;

        current_object = current_object->next_cousin;

        if (!current_object) {
            level--;
            if (!first_object->first_child)
                break;
            first_object = first_object->first_child;
            current_object = first_object;
        }
    }

    /* List of PUs */
    int max_os_index = 0;
    UT_array *ordered_host_array;
    int *ordered_hosts;
    utarray_new(ordered_host_array, &ut_int_icd);
    current_object = first_object;
    while (current_object) {
        max_os_index = (max_os_index >= current_object->os_index)?
            max_os_index: current_object->os_index;
        utarray_push_back(ordered_host_array, &current_object->os_index);
        current_object = current_object->next_cousin;
    }
    ordered_hosts = (int *)ordered_host_array->d;;

    /* Weight for the edges in the tree */
    int *weight_by_level = (int *)malloc((depth-1)*sizeof(int));
    for (int l = 0; l < depth-1; l++) {
        weight_by_level[l] = l+1;
    }

    netloc_arch_tree_t *tree = (netloc_arch_tree_t *)
        malloc(sizeof(netloc_arch_tree_t));
    tree->num_levels = depth-1;
    tree->degrees = max_down_degrees_by_level;
    tree->throughput = weight_by_level;

    int *arch_idx;
    int num_cores = utarray_len(ordered_host_array);
    netloc_complete_tree(tree, down_degrees_by_level, num_cores, &arch_idx);

    int *slot_idx = (int *)malloc(sizeof(int[max_os_index+1]));
    for (int i = 0; i < num_cores; i++) {
        slot_idx[ordered_hosts[i]] = arch_idx[i];
    }

    int num_leaves = netloc_tree_num_leaves(tree);
    int *slot_os_idx = (int *)malloc(sizeof(int[num_leaves]));
    for (int i = 0; i < num_cores; i++) {
        slot_os_idx[arch_idx[i]] = ordered_hosts[i];
    }

    arch->slot_tree = tree;
    arch->slot_idx = slot_idx;
    arch->slot_os_idx = slot_os_idx;
    arch->num_slots = max_os_index+1;

    return NETLOC_SUCCESS;
}
