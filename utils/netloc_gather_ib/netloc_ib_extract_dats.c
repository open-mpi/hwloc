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

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <uthash.h>
#include <utarray.h>

#include <libgen.h> // for dirname

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

typedef struct {
    UT_hash_handle hh;       /* makes this structure hashable */
    char physical_id[17];    /* key */
    char logical_id[17];
    char type[3];
    char subnet_id[20];
    char network_type[3];
    char description[];

} *node_t;
node_t nodes = NULL;

typedef struct {
    UT_hash_handle hh;         /* makes this structure hashable */
    char edge_id[19];             /* key */
    int ports[2];
    char port_types[2][3];
    char width[3];
    char speed[6];
    float gbits;
    char description[];
} *edge_t;
edge_t edges = NULL;

typedef struct {
    edge_t edge;
    int way;
} edge_ref_t;

UT_icd edge_ref_icd = {sizeof(edge_ref_t), NULL, NULL, NULL};

int read_routes(char *subnet, char *routes_path);
int read_discover(char *subnet, char *discover_path);

static void get_match(char *line, int nmatch, regmatch_t pmatch[], char *matches[])
{
    for (int i = 0; i < nmatch; i++) {
        regmatch_t current_match = pmatch[i];
        size_t len = current_match.rm_eo-current_match.rm_so;
        matches[i] = (char *)malloc((len+1)*sizeof(char));
        strncpy(matches[i], line+current_match.rm_so, len);
        matches[i][len] = '\0';
    }
}

node_t get_node(node_t *nodes, char *type, char *lid,
        char *guid, char *subnet, char *desc)
{
    node_t node;

    // TODO check guid format
    HASH_FIND_STR(*nodes, guid+2, node);  /* id already in the hash? */
    if (!node) {
        size_t size = sizeof(*node)+sizeof(char)*(strlen(desc)+1);
        node = (node_t) malloc(size);
        sprintf(node->physical_id, "%.17s", guid+2);
        sprintf(node->logical_id, "%.17s", lid);
        sprintf(node->type, "%.2s", type);
        sprintf(node->subnet_id, "%.19s", subnet);
        sprintf(node->network_type, "IB");
        sprintf(node->description, "%s", desc);

        HASH_ADD_STR(*nodes, physical_id, node);  /* guid: name of key field */
    }
    else
        printf("node found!\n");
    //printf("guid : %s\n", guid);
    return node;
}

static float compute_gbits(char *speed, char *width)
{
    float rate;
    float encoding;
    float gb_per_x;
    int x;

    if (strcmp(speed, "SDR")) {
        rate = 2.5;
        encoding = 8.0/10;
    } else if (strcmp(speed, "DDR")) {
        rate = 5;
        encoding = 8.0/10;
    } else if (strcmp(speed, "QDR")) {
        rate = 10;
        encoding = 8.0/10;
    } else if (strcmp(speed, "FDR")) {
        rate = 14.0625;
        encoding = 64.0/66;
    } else if (strcmp(speed, "FDR10")) {
        rate = 10;
        encoding = 64.0/66;
    } else if (strcmp(speed, "EDR")) {
        rate = 25;
        encoding = 64.0/66;
    } else {
        return 1;
    }
    gb_per_x = rate*encoding;

    regex_t width_re;
    regcomp(&width_re, "([[:digit:]]*)x", REG_EXTENDED);
    if (!regexec(&width_re, width, (size_t)0, NULL, 0)) {
        x = atoi(width);
    }
    else {
        return 0.0;
    }

    return x*gb_per_x;
 }

int main(int argc, char **argv)
{
    DIR *dir;

    char *path = argv[1];
    dir = opendir(path);
    if (dir != NULL) {
        regex_t subnet_regexp;
        // TODO
        //regcomp(&subnet_regexp, "^ib-subnet-([0-9a-fA-F:]{19}).txt$", REG_EXTENDED);
        regcomp(&subnet_regexp, "ib-subnet-([0-9a-fA-F:]{19}).txt$", REG_EXTENDED);
        struct dirent *entry;
        while ((entry = readdir(dir))) {
            int subnet_found;
            char *filename = entry->d_name;

            subnet_found = !(regexec(&subnet_regexp, filename, 0, NULL, REG_EXTENDED));
            if (subnet_found) {
                printf("current filename %s\n", filename);
                char *subnet = NULL;
                char *discover_path;
                char *routes_path;
                subnet = (char *)malloc(20*sizeof(char));
                strncpy(subnet, filename+10, 19);
                subnet[19] = '\0';

                asprintf(&discover_path, "%s/%s", path, filename);
                read_discover(subnet, discover_path);

                asprintf(&routes_path, "%s/ibroutes-%s", path, subnet);
                read_routes(subnet, routes_path);
                free(subnet);
            }
        }
        closedir(dir);
    }
    else
        perror ("Couldn't open the directory");

}

int read_discover(char *subnet, char *discover_path)
{
    char *line = NULL;
    size_t size = 0;
    FILE *discover_file = fopen(discover_path, "r");

    if (!discover_file) {
        perror("fopen: ");
        exit(-1);
    }

    regex_t dr_re;
    regcomp(&dr_re, "^DR", REG_EXTENDED);

    regex_t link_re;
    regcomp(&link_re, "^"
            "(CA|SW)[[:space:]]+"                // Source type
            "([[:digit:]]+)[[:space:]]+"         // Source lid
            "([[:digit:]]+)[[:space:]]+"         // Source port id
            "(0x[0-9a-f]{16})[[:space:]]+"       // Source guid
            "([[:digit:]]+x)[[:space:]]"         // Connection width
            "([^[:space:]]*)[[:space:]]+"        // Connection speed
            "-[[:space:]]+"                      // Dash seperator
            "(CA|SW)[[:space:]]+"                // Dest type
            "([[:digit:]]+)[[:space:]]+"         // Dest lid
            "([[:digit:]]+)[[:space:]]+"         // Dest port id
            "(0x[0-9a-f]{16})[[:space:]]+"       // Dest guid
            "\\([[:space:]]*(.*)[[:space:]]*\\)" // Description
            ,
            REG_EXTENDED);

    regex_t nolink_re;
    regcomp(&nolink_re, "^"
            "(CA|SW)[[:space:]]+"          // Source type
            "([[:digit:]]+)[[:space:]]+"   // Source lid
            "([[:digit:]]+)[[:space:]]+"   // Source port id
            "(0x[0-9a-f]{16})[[:space:]]+" // Source guid
            "([[:digit:]]+x)[[:space:]]"   // Connection width
            "([^[[:space:]]]*)"            // Connection speed
            ,
            REG_EXTENDED);

    int read;
    while ((read = getline(&line, &size, discover_file)) > 0) {
        const int link_nfields = 12;
        const int nolink_nfields = 7;
        const int max_nfields = 12;
        regmatch_t pmatch[max_nfields];
        char *matches[max_nfields];
        char *src_type;
        char *src_lid;
        char *src_port_id;
        char *src_guid;
        char *width;
        char *speed;
        char *dest_type;
        char *dest_lid;
        char *dest_port_id;
        char *dest_guid;
        char *edge_desc;
        char *src_desc;
        char *dest_desc;
        int have_peer;

        if (!regexec(&dr_re, line, (size_t)0, NULL, 0)) {
            /* DR line */
            continue;
        }
        else if (!regexec(&link_re, line, (size_t)link_nfields, pmatch, 0)) {
            /* peer associated: port is active */
            have_peer = 1;
            get_match(line, link_nfields, pmatch, matches);
            src_type     = matches[ 1];
            src_lid      = matches[ 2];
            src_port_id  = matches[ 3];
            src_guid     = matches[ 4];
            width        = matches[ 5];
            speed        = matches[ 6];
            dest_type    = matches[ 7];
            dest_lid     = matches[ 8];
            dest_port_id = matches[ 9];
            dest_guid    = matches[10];
            edge_desc    = matches[11];

            /* Analyse description */
            regex_t desc_re;
            regcomp(&desc_re, "(.*)" " - " "(.*)", REG_EXTENDED);
            if (!regexec(&desc_re, edge_desc, (size_t)3, pmatch, 0)) {
                get_match(edge_desc, 3, pmatch, matches);
                src_desc  = matches[1];
                dest_desc = matches[2];
            }
            else {
                src_desc = (char *)calloc(1, sizeof(char));
                dest_desc = (char *)calloc(1, sizeof(char));
            }

        }
        else if (!regexec(&nolink_re, line, (size_t)nolink_nfields, pmatch, 0)) {
            /* no peer associated: port is not active */
            have_peer = 0;
            get_match(line, nolink_nfields, pmatch, matches);
            src_type     = matches[ 1];
            src_lid      = matches[ 2];
            src_port_id  = matches[ 3];
            src_guid     = matches[ 4];
            width        = matches[ 5];
            speed        = matches[ 6];
        }
        else {
            printf("Warning: line not recognized: \n\t%s\n", line);
            continue;
        }

        /* Compute gbits */
        float gbits = compute_gbits(speed, width);

        /* Get the source node */
        node_t src_node =
            get_node(&nodes, src_type, src_lid, src_guid, subnet, src_desc);

        /* Add the link to the edge list */
        if (have_peer) {
            node_t dest_node =
                get_node(&nodes, dest_type, dest_lid, dest_guid, subnet, dest_desc);

            /* Compute the key */

            char *ref_node;
            char *ref_port;
            if (strcmp(dest_node->physical_id, src_node->physical_id) < 0) {
                ref_node = src_node->physical_id;
                ref_port = src_port_id;
            } else {
                ref_node = dest_node->physical_id;
                ref_port = dest_port_id;
            }

            char *edge_id;
            edge_t edge; 
            asprintf(&edge_id, "%s%s", ref_node, ref_port);

            int way;
            HASH_FIND_STR(edges, edge_id, edge);  /* id already in the hash? */
            if (!edge) {
                way = 0;
                size_t size = sizeof(*edge)+sizeof(char)*(strlen(edge_desc)+1);
                edge = (edge_t) malloc(size);
                strcpy(edge->edge_id, edge_id);

                edge->ports[0]      =  atoi(src_port_id);
                edge->ports[1]      =  atoi(dest_port_id);
                sprintf(edge->port_types[0], "%s", src_type);
                sprintf(edge->port_types[1], "%s", dest_type);
                sprintf(edge->width, "%s", width);
                sprintf(edge->speed, "%s", speed);
                edge->gbits = gbits;
                sprintf(edge->description, "%s", edge_desc);

                HASH_ADD_STR(edges, edge_id, edge);

                /* Add edge to the nodes */
                //edge_ref_t edge_ref;
                //edge_ref.edge = edge;
                //edge_ref.way = way;
                //utarray_push_back(src_node->edges, &edge_ref);
                //edge_ref.way = !way;
                //utarray_push_back(dest_node->edges, &edge_ref);
            } else {
                way = 1;
                /* Check info */
                // TODO change with warnings
                assert(edge->ports[0] ==  atoi(dest_port_id));
                assert(edge->ports[1] ==  atoi(src_port_id));
                assert(!strcmp(edge->port_types[0], dest_type));
                assert(!strcmp(edge->port_types[1], src_type));
                assert(!strcmp(edge->width,  width));
                assert(!strcmp(edge->speed,  speed));
                assert(edge->gbits ==  gbits);
            }
        }
    }
    if (read == -1 && errno) {
        perror("getline:");
        exit(-1);
    }
    free(line);

    FILE *output = fopen("/tmp/test.txt", "w");

    /* Write nodes to file */
    node_t node, node_tmp;
    HASH_ITER(hh, nodes, node, node_tmp) {
        fprintf(output, "%s,", node->physical_id);
        fprintf(output, "%s,", node->logical_id);
        fprintf(output, "%s,", node->type);
        fprintf(output, "%s,", node->subnet_id);
        fprintf(output, "%s,", node->network_type);
        fprintf(output, "%s", node->description);
        fprintf(output, "\n");
    }

    /* Write edges to file */
    edge_t edge, edge_tmp;
    HASH_ITER(hh, edges, edge, edge_tmp) {
        fprintf(output, "%s,", edge->edge_id);
        fprintf(output, "%d,", edge->ports[0]);
        fprintf(output, "%d,", edge->ports[1]);
        fprintf(output, "%s,", edge->port_types[0]);
        fprintf(output, "%s,", edge->port_types[1]);
        fprintf(output, "%s,", edge->width);
        fprintf(output, "%s,", edge->speed);
        fprintf(output, "%f,", edge->gbits);
        fprintf(output, "%s", edge->description);
        fprintf(output, "\n");
    }

    fclose(output);

    printf("done\n");
    return 0;
}

int read_routes(char *subnet, char *routes_path)
{
    DIR *dir;

    printf("Read subnet: %s\n", subnet);
    dir = opendir(routes_path);

    // TODO


    closedir(dir);

    return 0;
}
