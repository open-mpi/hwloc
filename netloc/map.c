//
// Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
// Copyright © 2013-2016 Inria.  All rights reserved.
// Copyright © 2013-2014 University of Wisconsin-La Crosse.
//
// See COPYING in top-level directory.
//
// $HEADER$
//

#include <netloc.h>
#include <netloc/map.h>
#include <private/netloc.h>
#include <private/map.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>


static void netloc_map__destroy_servers(struct netloc_map *map);
static void netloc_map__destroy_subnets(struct netloc_map *map);

static struct netloc_map__server * netloc_map__get_server_by_name(struct netloc_map *map, const char *name);
static struct netloc_map__subnet * netloc_map__get_subnet_by_id(struct netloc_map *map, netloc_network_type_t type, const char *id);
static struct netloc_map__port * netloc_map__get_port_by_id(struct netloc_map__subnet *subnet, const char *id);


/*****************************
 * Initializing a netloc_map
 */

int
netloc_map_create(netloc_map_t *mapp)
{
    struct netloc_map *map;

    map = calloc(sizeof(*map), 1);
    if (!map)
        goto out;

    const char *verbose_env;
    verbose_env = getenv("NETLOC_MAP_VERBOSE");
    if (verbose_env)
        map->verbose_flags = strtoul(verbose_env, NULL, 0);

    *mapp = map;
    return 0;

 out:
    return -1;
}

int
netloc_map_load_hwloc_data(netloc_map_t _map, const char *data_dir)
{
    struct netloc_map *map = _map;
    map->hwloc_xml_path = strdup(data_dir);
    return 0;
}

int
netloc_map_load_netloc_data(netloc_map_t _map, const char *data_dir)
{
    struct netloc_map *map = _map;
    map->netloc_data_path = strdup(data_dir);
    return 0;
}


/********************************
 * Managing hwloc topology diffs
 */

static int
netloc_map__prepare_hwloc_topology(struct netloc_map *map,
                                   struct netloc_map__server *server)
{
    if (map->flags & NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC
        && !server->topology) {
        unsigned i;
        int err;
        if (map->verbose_flags & NETLOC_MAP_VERBOSE_FLAG_COMPRESS)
            printf("uncompressing hwloc topology %s from %s\n",
                   server->name, server->topology_diff_refserver->name);
        err = hwloc_topology_dup(&server->topology, server->topology_diff_refserver->topology);
        if (err < 0)
            return -1;
        err = hwloc_topology_diff_apply(server->topology, server->topology_diff, 0);
        if (err < 0) {
            hwloc_topology_destroy(server->topology);
            server->topology = NULL;
            return -1;
        }
        for(i=0; i<server->nr_ports; i++)
            server->ports[i]->hwloc_obj = hwloc_get_obj_by_depth(server->topology, server->ports[i]->hwloc_obj_depth, server->ports[i]->hwloc_obj_index);
    }
    return 0;
}

static void
netloc_map__unprepare_hwloc_topology(struct netloc_map *map,
                                     struct netloc_map__server *server)
{
    if (map->flags & NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC
        && server->topology_diff) {
        unsigned i;
        if (map->verbose_flags & NETLOC_MAP_VERBOSE_FLAG_COMPRESS)
            printf("compressing hwloc topology %s on top of %s\n",
                   server->name, server->topology_diff_refserver->name);
        hwloc_topology_destroy(server->topology);
        server->topology = NULL;
        for(i=0; i<server->nr_ports; i++)
            server->ports[i]->hwloc_obj = NULL;
    }
}


/*************************
 * Building a netloc_map
 */

static int
netloc_map__init_subnets(struct netloc_map *map)
{
    int err;
    netloc_network_t **nets = NULL;
    int nrnets = 0, i;

    if (!map->netloc_data_path)
        return 0;

    netloc_foreach_network((const char * const *) &map->netloc_data_path, 1, NULL, map, &nrnets, &nets);

    err = netloc_lookup_table_init(&map->subnet_by_id[NETLOC_NETWORK_TYPE_ETHERNET], nrnets,
                                   NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);
    if (NETLOC_SUCCESS != err)
        goto out;

    err = netloc_lookup_table_init(&map->subnet_by_id[NETLOC_NETWORK_TYPE_INFINIBAND], nrnets,
                                   NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);
    if (NETLOC_SUCCESS != err)
        goto out_with_ltable_ethernet;

    for(i=0; i<nrnets; i++) {
        netloc_topology_t netloc;
        err = netloc_attach(&netloc, *nets[i]);
        if (!err) {
            struct netloc_map__subnet *subnet;

            subnet = calloc(sizeof(*subnet) + strlen(nets[i]->subnet_id)+1, 1);
            if (!subnet) {
                netloc_detach(netloc);
                continue;
            }

            strcpy(subnet->id, nets[i]->subnet_id);
            subnet->topology = netloc;
            subnet->type = nets[i]->network_type;

            err = netloc_lookup_table_append(&map->subnet_by_id[nets[i]->network_type], subnet->id, subnet);
            if (NETLOC_SUCCESS != err) {
                free(subnet);
                netloc_detach(netloc);
                continue;
            }

            subnet->next = NULL;
            subnet->prev = map->subnet_last;
            if (map->subnet_last)
                map->subnet_last->next = subnet;
            else
                map->subnet_first = subnet;
            map->subnet_last = subnet;
            map->subnets_nr++;
        }
        netloc_dt_network_t_destruct(nets[i]);
    }

    free(nets);
    return 0;

 out_with_ltable_ethernet:
    netloc_lookup_table_destroy(&map->subnet_by_id[NETLOC_NETWORK_TYPE_ETHERNET]);
    memset(&map->subnet_by_id, 0, sizeof(map->subnet_by_id));
 out:
    return -1;
}

static
void netloc_map__add_port(struct netloc_map *map,
                          struct netloc_map__server *server,
                          netloc_network_type_t type,
                          const char * gid,
                          hwloc_obj_t obj)
{
    struct netloc_map__port *port;
    struct netloc_map__subnet *subnet;
    const char *id;
    size_t id_len;

    if (!gid)
        return;

    if (type == NETLOC_NETWORK_TYPE_ETHERNET) {
        subnet = netloc_map__get_subnet_by_id(map, type, "unknown");
        id = gid;
        id_len = 17;
    } else if (type == NETLOC_NETWORK_TYPE_INFINIBAND) {
        char subnet_id[20];
        memcpy(subnet_id, gid, 19);
        subnet_id[19] = '\0';
        subnet = netloc_map__get_subnet_by_id(map, type, subnet_id);
        id = gid+20;
        id_len = 19;
    }
    else {
        subnet = NULL;
    }

    if (!subnet)
        return;

    if (server->nr_ports == server->nr_ports_allocated) {
        unsigned new_nr_ports_allocated = server->nr_ports_allocated ? 2*server->nr_ports_allocated : 8;
        struct netloc_map__port **new_ports;
        new_ports = malloc(new_nr_ports_allocated * sizeof(*new_ports));
        if (!new_ports)
            return;
        memcpy(new_ports, server->ports, server->nr_ports * sizeof(*new_ports));
        free(server->ports);
        server->ports = new_ports;
        server->nr_ports_allocated = new_nr_ports_allocated;
    }

    port = calloc(sizeof(*port) + id_len + 1, 1);
    if (!port)
        return;
    strcpy(port->id, id);
    port->server = server;
    port->subnet = subnet;

    port->hwloc_obj = obj;
    port->hwloc_obj_depth = obj->depth;
    port->hwloc_obj_index = obj->logical_index;

    server->ports[server->nr_ports++] = port;

    map->server_ports_nr++;
}

static void
netloc_map__add_ib_port(struct netloc_map *map,
                        struct netloc_map__server *server,
                        hwloc_obj_t obj)
{
    /* scan infos, and add valid ports using GIDs */
    int currentport = -1;
    int currentportvalid = 1;
    char * currentportgid = NULL;
    unsigned i;
    for(i=0; i<obj->infos_count; i++) {
        struct hwloc_obj_info_s *info = &obj->infos[i];
        unsigned newport, gid, pos;

        /* check port number */
        if (sscanf(info->name, "Port%u%n", &newport, &pos) >= 1) {
            /* new port ? */
            if ((int) newport != currentport) {
                /* try to add previous port */
                if (currentport != -1 && currentportvalid)
                    netloc_map__add_port(map, server, NETLOC_NETWORK_TYPE_INFINIBAND, currentportgid, obj);
                /* reset new port */
                currentport = newport; currentportvalid = 1; currentportgid = NULL;
            }
            if (!strcmp(info->name+pos, "State")) {
                /* active PortState defined in IB spec, section 14.2.5.6 "Port Info" */
                unsigned state = atoi(info->value);
                if (state != 4)
                    currentportvalid = 0;
            } else if (!strcmp(info->name+pos, "LID")) {
                /* unicast lid range defined in IB spec, section 4.1.3 "Local Identifiers", rule 9 */
                unsigned lid = strtoul(info->value, NULL, 0);
                if (lid < 0x1 || lid > 0xbfff)
                    currentportvalid = 0;
            } else if (sscanf(info->name+pos, "GID%u", &gid) == 1) {
                /* gid */
                currentportgid = info->value;
            }
        }
    }
    /* queue last port */
    if (currentport != -1 && currentportvalid)
        netloc_map__add_port(map, server, NETLOC_NETWORK_TYPE_INFINIBAND, currentportgid, obj);
}

static void
netloc_map__add_eth_port(struct netloc_map *map,
                         struct netloc_map__server *server,
                         hwloc_obj_t obj)
{
    unsigned i;
    for(i=0; i<obj->infos_count; i++) {
        struct hwloc_obj_info_s *info = &obj->infos[i];

        if (!strcmp(info->name, "Address")) {
            netloc_map__add_port(map, server, NETLOC_NETWORK_TYPE_ETHERNET, info->value, obj);
            break;
        }
    }
}

static struct netloc_map__server *
netloc_map__init_server(struct netloc_map *map, hwloc_topology_t topo, const char *name)
{
    struct netloc_map__server *server;
    hwloc_obj_t obj;
    const char *infoname;
    size_t namelen;
    int err;

    /* setup a "HostName" info attr in the topology to recognize the name from the topology */
    obj = hwloc_get_root_obj(topo);
    assert(obj);
    infoname = hwloc_obj_get_info_by_name(obj, "HostName");
    if (!infoname)
        hwloc_obj_add_info(obj, "HostName", name);
    else
        name = infoname;
    namelen = strlen(name);

    server = calloc(sizeof(*server) + namelen+1, 1);
    if (!server)
        return NULL;

    strcpy(server->name, name);
    server->topology = topo;
    hwloc_topology_set_userdata(topo, server);

    if (netloc_lookup_table_access(&map->server_by_name, server->name)) {
        fprintf(stderr, "Found duplicate server %s, ignoring the new one\n", server->name);
        free(server);
        return NULL;
    }

    err = netloc_lookup_table_append(&map->server_by_name, server->name, server);
    if (NETLOC_SUCCESS != err) {
        free(server);
        return NULL;
    }
    server->map = map;

    server->next = NULL;
    server->prev = map->server_last;
    if (map->server_last)
        map->server_last->next = server;
    else
        map->server_first = server;
    map->server_last = server;
    map->servers_nr++;

    obj = NULL;
    while ((obj = hwloc_get_next_osdev(topo, obj)) != NULL) {
        if (obj->attr->osdev.type == HWLOC_OBJ_OSDEV_OPENFABRICS) {
            netloc_map__add_ib_port(map, server, obj);
        } else if (obj->attr->osdev.type == HWLOC_OBJ_OSDEV_NETWORK) {
            netloc_map__add_eth_port(map, server, obj);
        }
    }

    return server;
}

static int
netloc_map__init_servers(struct netloc_map *map)
{
    DIR *hwloc_xml_dir;
    struct dirent *dirent;
    unsigned nbxmls;
    unsigned founddiffs = 0;
    int err;

    if (!map->hwloc_xml_path)
        return 0;

    hwloc_xml_dir = opendir(map->hwloc_xml_path);
    if (!hwloc_xml_dir)
        goto out;

    nbxmls=0;
    while ((dirent = readdir(hwloc_xml_dir)) != NULL)
        nbxmls++;
    rewinddir(hwloc_xml_dir);

    err = netloc_lookup_table_init(&map->server_by_name, nbxmls,
                                   NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);
    if (NETLOC_SUCCESS != err)
        goto out_with_dir;

    /* load uncompressed topologies */
    while ((dirent = readdir(hwloc_xml_dir)) != NULL) {
        struct netloc_map__server *server;
        hwloc_topology_t topo;
        char *name;
        char *filepath;
        size_t namelen;
        int err;

        name = dirent->d_name;
        namelen = strlen(name);
        if (namelen < 4)
            continue;
        if (strcmp(".xml", name+namelen-4))
            continue;
        namelen -= 4;
        name[namelen] = '\0';

        if (namelen >= 5 && !strcmp(".diff", name+namelen-5)) {
            founddiffs++;
            continue;
        }

        err = asprintf(&filepath, "%s/%s.xml", map->hwloc_xml_path, name);
        if (err < 0)
            continue;

        hwloc_topology_init(&topo);
	hwloc_topology_set_type_filter(topo, HWLOC_OBJ_PCI_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
	hwloc_topology_set_type_filter(topo, HWLOC_OBJ_OS_DEVICE, HWLOC_TYPE_FILTER_KEEP_IMPORTANT);
	/* no need for bridges */
        err = hwloc_topology_set_xml(topo, filepath);
        if (err < 0) {
	  fprintf(stderr, "hwloc_topology_set_xml() failed on XML `%s' (%s), ignoring this file.\n", filepath, strerror(errno));
	  free(filepath);
	  hwloc_topology_destroy(topo);
	  continue;
        }
        err = hwloc_topology_load(topo);
	if (err < 0) {
	  fprintf(stderr, "hwloc_topology_load() failed on XML `%s' (%s), ignoring this file.\n", filepath, strerror(errno));
	  free(filepath);
	  hwloc_topology_destroy(topo);
	  continue;
	}
        free(filepath);

        server = netloc_map__init_server(map, topo, name);
        if (!server)
            hwloc_topology_destroy(topo);

        /* FIXME: try to compress if the input directory isn't ? */
    }

    /* load compressed topologies now that the refs are ready */
    if (founddiffs) {
        /* find one valid topology so that we can load diffs */
        if (!map->server_first) {
            fprintf(stderr, "Found %u hwloc topology diffs, cannot load without any entire topology\n",
                    founddiffs);
        } else {
            hwloc_topology_t validtopo = map->server_first->topology;

            /* now reread the directory entries */
            rewinddir(hwloc_xml_dir);
            while ((dirent = readdir(hwloc_xml_dir)) != NULL) {
                struct netloc_map__server *server, *refserver;
                hwloc_topology_t topo;
                hwloc_topology_diff_t diff;
                char *filepath;
                char *refname;
                size_t refnamelen;
                char *name;
                size_t namelen;
                int err;

                name = dirent->d_name;
                namelen = strlen(name);
                if (namelen < 9)
                    continue;
                if (strcmp(".diff.xml", name+namelen-9))
                    continue;
                namelen -= 9;
                name[namelen] = '\0';

                err = asprintf(&filepath, "%s/%s.diff.xml", map->hwloc_xml_path, name);
                if (err < 0)
                    continue;

                if (map->verbose_flags & NETLOC_MAP_VERBOSE_FLAG_COMPRESS)
                    printf("loading topology diff %s\n", filepath);

                err = hwloc_topology_diff_load_xml(filepath, &diff, &refname);
                free(filepath);
                if (err < 0)
                    continue;
                if (!refname) {
                    hwloc_topology_diff_destroy(diff);
                    continue;
                }
                refnamelen = strlen(refname);
                if (strcmp(refname+refnamelen-4, ".xml")) {
                    free(refname);
                    hwloc_topology_diff_destroy(diff);
                    continue;
                }
                refname[refnamelen-4] = '\0';

                if (map->verbose_flags & NETLOC_MAP_VERBOSE_FLAG_COMPRESS)
                    printf("  applying diff on top of reference topology %s\n", refname);

                refserver = netloc_lookup_table_access(&map->server_by_name, refname);
                if (!refserver) {
                    fprintf(stderr, "Could not find hwloc topology diff reference server %s\n", refname);
                    free(refname);
                    hwloc_topology_diff_destroy(diff);
                    continue;
                }

                /* make sure the ref isn't compressed */
                if (netloc_map__prepare_hwloc_topology(map, refserver) < 0) {
                    fprintf(stderr, "Failed to uncompress reference server %s topology\n", refname);
                    free(refname);
                    hwloc_topology_diff_destroy(diff);
                    continue;
                }
                free(refname);

                err = hwloc_topology_dup(&topo, refserver->topology);
                if (err < 0) {
                    hwloc_topology_diff_destroy(diff);
                    continue;
                }

                err = hwloc_topology_diff_apply(topo, diff, 0);
                if (err < 0) {
                    hwloc_topology_diff_destroy(diff);
                    hwloc_topology_destroy(topo);
                    continue;
                }

                server = netloc_map__init_server(map, topo, name);
                if (!server) {
                    hwloc_topology_diff_destroy(diff);
                    hwloc_topology_destroy(topo);
                } else {
                    /* ideally, we would walk up the chain of reference servers, but:
                     * - there shouldn't be any stack of multiple diffs if the compression is properly done.
                     * - things could fail above if the diffs are not loaded in the right order.
                     * so we don't bother.
                     */
                    server->topology_diff_refserver = refserver;
                    refserver->usecount++;
                    server->topology_diff = diff;
                    /* compress the new server, there will likely be nobody depending on it */
                    netloc_map__unprepare_hwloc_topology(map, server);
                }
            }
        }
    }

    closedir(hwloc_xml_dir);
    return 0;

 out_with_dir:
    closedir(hwloc_xml_dir);
 out:
    return -1;
}

static int
netloc_map__prepare_subnet_port_hashes(struct netloc_map *map)
{
    struct netloc_map__subnet *subnet;
    int err;

    subnet = map->subnet_first;
    while (subnet) {
        err = netloc_lookup_table_init(&subnet->port_by_id, map->server_ports_nr,
                                       NETLOC_LOOKUP_TABLE_FLAG_NO_STRDUP_KEY);
        if (NETLOC_SUCCESS != err)
            return -1;
        subnet->port_by_id_ready = 1;
        subnet = subnet->next;
    }
    return 0;
}

static int
netloc_map__hash_ports(struct netloc_map *map)
{
    struct netloc_map__server *server;
    int err;

    server = map->server_first;
    while (server) {
        unsigned i;
        for(i=0; i<server->nr_ports; i++) {
            struct netloc_map__port *port = server->ports[i];
            struct netloc_map__subnet *subnet = port->subnet;

            err = netloc_lookup_table_append(&subnet->port_by_id, port->id, port);
            if (NETLOC_SUCCESS != err)
                return -1;

            port->next = NULL;
            port->prev = subnet->port_last;
            if (subnet->port_last)
                subnet->port_last->next = port;
            else
                subnet->port_first = port;
            subnet->port_last = port;
            subnet->ports_nr++;
        }
        server = server->next;
    }

    return 0;
}

static int
netloc_map__map(struct netloc_map *map)
{
    struct netloc_map__subnet *subnet;

    subnet = map->subnet_first;
    while (subnet) {
        struct netloc_map__port *port = subnet->port_first;
        while (port) {
            netloc_node_t *node;

            node = netloc_get_node_by_physical_id(subnet->topology, port->id);

            if (node) {
                netloc_edge_t **edges = NULL;
                int nredges = 0;
                int err;

                err = netloc_get_all_edges(subnet->topology, node, &nredges, &edges);
                if (NETLOC_SUCCESS == err) {
                    assert(nredges == 1);
                    port->edge = edges[0];
                    assert(!strcmp(port->edge->src_node_id, port->id));
                }
            }

            port = port->next;
        }
        subnet = subnet->next;
    }
    return 0;
}

int
netloc_map_build(netloc_map_t _map, unsigned long flags)
{
    struct netloc_map *map = _map;
    int err;

    if (flags & ~NETLOC_MAP_BUILD_FLAG_COMPRESS_HWLOC) {
        errno = EINVAL;
        return -1;
    }

    map->flags = flags;

    err = netloc_map__init_subnets(map);
    if (err < 0)
        goto out;
    err = netloc_map__init_servers(map);
    if (err < 0)
        goto out;
    err = netloc_map__prepare_subnet_port_hashes(map);
    if (err < 0)
        goto out;
    err = netloc_map__hash_ports(map);
    if (err < 0)
        goto out;
    err = netloc_map__map(map);
    if (err < 0)
        goto out;

    map->merged = true;
    return 0;

 out:
    netloc_map__destroy_subnets(map);
    netloc_map__destroy_servers(map);
    return -1;
}


/************************
 * Destroy a netloc_map
 */

static void
netloc_map__destroy_servers(struct netloc_map *map)
{
    struct netloc_map__server *curserver, *nextserver;
    unsigned i;

    curserver = map->server_first;
    while (curserver) {
        nextserver = curserver->next;
        if (curserver->topology_diff_refserver)
            hwloc_topology_diff_destroy(curserver->topology_diff);
        if (curserver->topology)
            hwloc_topology_destroy(curserver->topology);
        for(i=0; i<curserver->nr_ports; i++)
            free(curserver->ports[i]);
        free(curserver->ports);
        free(curserver);
        curserver = nextserver;
    }
    map->server_first = map->server_last = NULL;
    map->servers_nr = 0;

    netloc_lookup_table_destroy(&map->server_by_name);
    memset(&map->server_by_name, 0, sizeof(map->server_by_name));
}

static void
netloc_map__destroy_subnets(struct netloc_map *map)
{
    struct netloc_map__subnet *cursubnet, *nextsubnet;

    cursubnet = map->subnet_first;
    while (cursubnet) {
        nextsubnet = cursubnet->next;
        netloc_detach(cursubnet->topology);
        if (cursubnet->port_by_id_ready)
            netloc_lookup_table_destroy(&cursubnet->port_by_id);
        free(cursubnet);
        cursubnet = nextsubnet;
    }
    map->subnet_first = map->subnet_last = NULL;
    map->subnets_nr = 0;

    netloc_lookup_table_destroy(&map->subnet_by_id[NETLOC_NETWORK_TYPE_ETHERNET]);
    netloc_lookup_table_destroy(&map->subnet_by_id[NETLOC_NETWORK_TYPE_INFINIBAND]);
    memset(&map->subnet_by_id, 0, sizeof(map->subnet_by_id));
}

int
netloc_map_destroy(netloc_map_t _map)
{
    struct netloc_map *map = _map;

    free(map->netloc_data_path);
    free(map->hwloc_xml_path);

    if (map->merged) {
        netloc_map__destroy_subnets(map);
        netloc_map__destroy_servers(map);
    }

    free(map);
    return 0;
}


/*******************
 * Internal helpers
 */

static struct netloc_map__server *
netloc_map__get_server_by_name(struct netloc_map *map,
                               const char *name)
{
    return (struct netloc_map__server *) netloc_lookup_table_access(&map->server_by_name, name);
}

static struct netloc_map__server *
netloc_map__get_server_by_topology(struct netloc_map *map __netloc_attribute_unused,
                                   hwloc_topology_t topology)
{
    return hwloc_topology_get_userdata(topology);
}

static struct netloc_map__subnet *
netloc_map__get_subnet_by_id(struct netloc_map *map,
                             netloc_network_type_t type,
                             const char *id)
{
    return (struct netloc_map__subnet *) netloc_lookup_table_access(&map->subnet_by_id[type], id);
}

static struct netloc_map__port *
netloc_map__get_port_by_id(struct netloc_map__subnet *subnet,
                           const char *id)
{
    return (struct netloc_map__port *) netloc_lookup_table_access(&subnet->port_by_id, id);
}

static int
netloc_map__get_hwloc_topology(struct netloc_map__server *server)
{
    if (!server->usecount)
        if (netloc_map__prepare_hwloc_topology(server->map, server) < 0)
            return -1;
    server->usecount++;
    return 0;
}

static void
netloc_map__put_hwloc_topology(struct netloc_map *map,
                               struct netloc_map__server *server)
{
    if (!--server->usecount)
        netloc_map__unprepare_hwloc_topology(map, server);
}

static int
netloc_map__check_port_locality(struct netloc_map__port *port,
                                hwloc_obj_t closeto_obj)
{
    hwloc_obj_t port_obj = port->hwloc_obj;

    assert(port_obj->type == HWLOC_OBJ_OS_DEVICE);

    if (!closeto_obj)
        /* no closeto_obj specified, all objects match */
        return 1;

    /* if we want to be close to a OS device, it must be the exact port object */
    if (closeto_obj->type == HWLOC_OBJ_OS_DEVICE)
        return closeto_obj == port_obj;

    /* if we want to be close to a PCI device or bridge, it must be a parent of the port object */
    if (closeto_obj->type == HWLOC_OBJ_PCI_DEVICE
        || closeto_obj->type == HWLOC_OBJ_BRIDGE) {
        while (port_obj) {
            if (port_obj == closeto_obj)
                return 1;
            port_obj = port_obj->parent;
        }
        return 0;
    }

    /* if we want to be close to something with no locality (multiple-node topology?!), all ports match */
    if (!closeto_obj->cpuset)
        return 1;

    /* we want to be close to some PUs, check that the port locality intersects them */
    port_obj = hwloc_get_non_io_ancestor_obj(port->server->topology, port_obj);
    return hwloc_bitmap_intersects(closeto_obj->cpuset, port_obj->cpuset);
}


/*****************
 * Public queries
 */

int netloc_map_hwloc2port(netloc_map_t _map,
                          hwloc_topology_t htopo, hwloc_obj_t hobj,
                          netloc_map_port_t *portsp, unsigned *nrp)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;
    unsigned i, found, room = *nrp;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    server = netloc_map__get_server_by_topology(map, htopo);
    if (!server) {
        errno = EINVAL;
        return -1;
    }

    found = 0;
    for(i=0; i<server->nr_ports; i++) {
        struct netloc_map__port *port = server->ports[i];
        if (!netloc_map__check_port_locality(port, hobj))
            continue;

        found++;
        if (room) {
            *(portsp++) = port;
            room--;
        }
    }

    *nrp -= room;
    return found;
}

int
netloc_map_port2hwloc(netloc_map_port_t _port,
                      hwloc_topology_t *htopop, hwloc_obj_t *hobjp)
{
    struct netloc_map__port *port = _port;

    if (!port->server->map->merged || !htopop) {
        errno = EINVAL;
        return -1;
    }

    *htopop = port->server->topology;
    if (hobjp)
        *hobjp = port->hwloc_obj;
    return 0;
}

int netloc_map_netloc2port(netloc_map_t _map,
                           netloc_topology_t ntopo __netloc_attribute_unused, netloc_node_t *nnode, netloc_edge_t *nedge,
                           netloc_map_port_t *portp)
{
    struct netloc_map *map = _map;
    struct netloc_map__subnet *subnet;
    struct netloc_map__port *port;
    const char *physical_id = NULL;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    if (nedge) {
        if (nnode) {
            /* both node and edge:
             * check they are connected, use node id.
             */
            if ((NETLOC_NODE_TYPE_HOST == nedge->src_node_type && !strcmp(nedge->src_node_id, nnode->physical_id))
                || (NETLOC_NODE_TYPE_HOST == nedge->dest_node_type && !strcmp(nedge->dest_node_id, nnode->physical_id)))
                physical_id = nnode->physical_id;
        } else {
            /* edge without node:
             * find the node id from the edge, must have a single HOST vertex.
             */
            if (NETLOC_NODE_TYPE_HOST == nedge->src_node_type
                && NETLOC_NODE_TYPE_HOST != nedge->dest_node_type)
                physical_id = nedge->src_node_id;
            else if (NETLOC_NODE_TYPE_HOST != nedge->src_node_type
                     && NETLOC_NODE_TYPE_HOST == nedge->dest_node_type)
                physical_id = nedge->dest_node_id;
        }
    } else {
        if (nnode) {
            /* node without edge:
             * use the node id
             */
            physical_id = nnode->physical_id;
        }
    }
    if (!physical_id) {
        errno = EINVAL;
        return -1;
    }

    subnet = netloc_map__get_subnet_by_id(map,
					  nnode ? nnode->network_type : nedge->src_node->network_type,
					  nnode ? nnode->subnet_id : nedge->src_node->subnet_id);
    if (!subnet)
        return -1;

    port = netloc_map__get_port_by_id(subnet, physical_id);
    if (!port)
        return -1;

    *portp = port;
    return 0;
}

int
netloc_map_port2netloc(netloc_map_port_t _port,
                       netloc_topology_t *ntopo, netloc_node_t **nnode, netloc_edge_t **nedge)
{
    struct netloc_map__port *port = _port;

    if (!port->server->map->merged) {
        errno = EINVAL;
        return -1;
    }

    if (ntopo)
        *ntopo = port->subnet->topology;
    if (nedge)
        *nedge = port->edge;
    if (nnode)
        *nnode = port->edge ? port->edge->src_node : NULL;
    return 0;
}

int
netloc_map_server2hwloc(netloc_map_server_t _server,
                        hwloc_topology_t *topologyp)
{
    struct netloc_map__server *server = _server;

    if (!server->map->merged || !topologyp) {
        errno = EINVAL;
        return -1;
    }

    if (netloc_map__get_hwloc_topology(server) < 0)
        return -1;

    *topologyp = server->topology;
    return 0;
}

int
netloc_map_hwloc2server(netloc_map_t _map,
                        hwloc_topology_t topology,
                        netloc_map_server_t *serverp)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    server = netloc_map__get_server_by_topology(map, topology);
    if (!server) {
        errno = EINVAL;
        return -1;
    }

    *serverp = server;
    return 0;
}

int
netloc_map_put_hwloc(netloc_map_t _map,
                     hwloc_topology_t topology)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    server = netloc_map__get_server_by_topology(map, topology);
    if (!server) {
        errno = EINVAL;
        return -1;
    }

    netloc_map__put_hwloc_topology(map, server);
    return 0;
}

int netloc_map_get_subnets(netloc_map_t _map,
                           unsigned *nr,
                           netloc_topology_t **toposp)
{
    struct netloc_map *map = _map;
    netloc_topology_t *topos;
    struct netloc_map__subnet *subnet;
    unsigned i;

    topos = malloc(map->subnets_nr * sizeof(*topos));
    if (!topos)
        return -1;

    i = 0;
    subnet = map->subnet_first;
    while (subnet) {
        topos[i] = subnet->topology;
        i++;
        subnet = subnet->next;
    }

    *toposp = topos;
    *nr = map->subnets_nr;
    return 0;
}

int netloc_map_get_nbservers(netloc_map_t _map)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;
    unsigned i;

    server = map->server_first;
    i=0;
    while (server) {
        server = server->next;
        i++;
    }

    return (int) i;
}

int netloc_map_get_servers(netloc_map_t _map,
                           unsigned first, unsigned nr,
                           netloc_map_server_t servers[])
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;
    unsigned i;

    if (!nr)
        return 0;

    server = map->server_first;
    while (server && first) {
        server = server->next;
        first--;
    }
    if (!server) {
        errno = ERANGE;
        return -1;
    }

    i = 0;
    while (server && i<nr) {
        servers[i] = server;
        server = server->next;
        i++;
    }

    return (int) i;
}

int netloc_map_get_server_ports(netloc_map_server_t _server,
                                unsigned *nr, netloc_map_port_t **portsp)
{
    struct netloc_map__server *server = _server;
    *nr = server->nr_ports;
    *portsp = (netloc_map_port_t *) server->ports;
    return 0;
}

int netloc_map_port2server(netloc_map_port_t _port,
                           netloc_map_server_t *server)
{
    struct netloc_map__port *port = _port;
    *server = port->server;
    return 0;
}

int netloc_map_server2map(netloc_map_server_t _server,
                          netloc_map_t *map)
{
    struct netloc_map__server *server = _server;
    *map = server->map;
    return 0;
}

int netloc_map_server2name(netloc_map_server_t _server,
                           const char **namep)
{
    struct netloc_map__server *server = _server;

    if (!server->map->merged) {
        errno = EINVAL;
        return -1;
    }

    *namep = server->name;
    return 0;
}

int netloc_map_name2server(netloc_map_t _map,
                           const char *name, netloc_map_server_t *serverp)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    server = netloc_map__get_server_by_name(map, name);
    if (!server) {
        errno = EINVAL;
        return -1;
    }

    *serverp = server;
    return 0;
}

/*****************************
 * Paths
 */

static int
netloc_map__paths_build_hwloc_edge_to_port(hwloc_topology_t topology,
                                           hwloc_obj_t src, hwloc_obj_t port,
                                           unsigned long flags,
                                           struct netloc_map_edge_s **edgesp, unsigned *nr_edgesp)
{
    struct netloc_map_edge_s *edges = *edgesp;
    unsigned nr_edges = *nr_edgesp;
    unsigned i = nr_edges;
    unsigned pcilength;
    hwloc_obj_t portparent, srcparent, srccousin, ancestor;

    nr_edges += 4;
    edges = realloc(edges, nr_edges * sizeof(*edges));
    if (!edges) {
        *edgesp = NULL;
        *nr_edgesp = 0;
        return -1;
    }

    /*  hwloc_get_non_io_ancestor_obj(topology, port) with length */
    pcilength = 0;
    portparent = port;
    while (!portparent->cpuset) {
        if (!portparent->parent)
            break;
        portparent = portparent->parent;
        pcilength++;
    }

    if (portparent->depth > src->depth) {
        /*  src ------ srccousin
            |
            portparent
            |
            port
        */

        /* find portparent cousin below src */
        srccousin = portparent;
        while (srccousin->depth != src->depth)
            srccousin = srccousin->parent;
        if (srccousin != src) {
            /* go horizontal to srccousin */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL;
            edges[i].hwloc.src_obj = src;
            edges[i].hwloc.dest_obj = srccousin;
            /* find the weight through common ancestor */
            ancestor = hwloc_get_common_ancestor_obj(topology, src, srccousin);
            edges[i].hwloc.weight = (src->depth - ancestor->depth) * 2 - 1;
            i++;
        }
        /* go down to portparent */
        if (portparent != srccousin && (flags & NETLOC_MAP_PATHS_FLAG_VERTICAL)) {
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_CHILD;
            edges[i].hwloc.src_obj = srccousin;
            edges[i].hwloc.dest_obj = portparent;
            edges[i].hwloc.weight = src->depth - portparent->depth;
            i++;
        }
    } else {
        /*  srcparent ----- portparent
            |               |
            src              |
            port
        */

        /* find portparent cousin above src */
        srcparent = src;
        while (srcparent->depth != portparent->depth)
            srcparent = srcparent->parent;
        if (srcparent != src && (flags & NETLOC_MAP_PATHS_FLAG_VERTICAL)) {
            /* go up from src to srcparent */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_PARENT;
            edges[i].hwloc.src_obj = src;
            edges[i].hwloc.dest_obj = srcparent;
            edges[i].hwloc.weight = src->depth - srcparent->depth;
            i++;
        }
        if (srcparent != portparent) {
            /* go horizontal from srcparent to portparent if different */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL;
            edges[i].hwloc.src_obj = srcparent;
            edges[i].hwloc.dest_obj = portparent;
            /* find the weight through common ancestor */
            ancestor = hwloc_get_common_ancestor_obj(topology, portparent, srcparent);
            edges[i].hwloc.weight = (portparent->depth - ancestor->depth) * 2 - 1;
            i++;
        }
    }

    if (flags & NETLOC_MAP_PATHS_FLAG_IO) {
        /* now go down from portparent to port */
        edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_PCI;
        edges[i].hwloc.src_obj = portparent;
        edges[i].hwloc.dest_obj = port;
        edges[i].hwloc.weight = pcilength;
        i++;
    }

    *edgesp = edges;
    *nr_edgesp = i;
    return 0;
}

static int
netloc_map__paths_build_hwloc_edge_from_port(hwloc_topology_t topology,
                                             hwloc_obj_t port, hwloc_obj_t dst,
                                             unsigned long flags,
                                             struct netloc_map_edge_s **edgesp, unsigned *nr_edgesp)
{
    struct netloc_map_edge_s *edges = *edgesp;
    unsigned nr_edges = *nr_edgesp;
    unsigned i = nr_edges;
    hwloc_obj_t portparent, dstparent, dstcousin, ancestor;
    unsigned pcilength;

    nr_edges += 3;
    edges = realloc(edges, nr_edges * sizeof(*edges));
    if (!edges) {
        *edgesp = NULL;
        *nr_edgesp = 0;
        return -1;
    }

    /*  hwloc_get_non_io_ancestor_obj(topology, port) with length */
    pcilength = 0;
    portparent = port;
    while (!portparent->cpuset) {
        if (!portparent->parent)
            break;
        portparent = portparent->parent;
        pcilength++;
    }

    if (flags & NETLOC_MAP_PATHS_FLAG_IO) {
        /* go up from portparent to port */
        edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_PCI;
        edges[i].hwloc.src_obj = port;
        edges[i].hwloc.dest_obj = portparent;
        edges[i].hwloc.weight = pcilength;
        i++;
    }

    if (portparent->depth > dst->depth) {
        /*  dstcousin ------ dst
            |
            portparent
            |
            port
        */

        /* find portparent cousin below dst */
        dstcousin = portparent;
        while (dstcousin->depth != dst->depth)
            dstcousin = dstcousin->parent;
        /* go up to dstcousin */
        if (portparent != dst && (flags & NETLOC_MAP_PATHS_FLAG_VERTICAL)) {
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_PARENT;
            edges[i].hwloc.src_obj = portparent;
            edges[i].hwloc.dest_obj = dstcousin;
            edges[i].hwloc.weight = dst->depth - portparent->depth;
            i++;
        }
        if (dstcousin != dst) {
            /* go horizontal to dst */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL;
            edges[i].hwloc.src_obj = dstcousin;
            edges[i].hwloc.dest_obj = dst;
            /* find the weight through common ancestor */
            ancestor = hwloc_get_common_ancestor_obj(topology, dstcousin, dst);
            edges[i].hwloc.weight = (dst->depth - ancestor->depth) * 2 - 1;
            i++;
        }
    } else {
        /*  portparent ----- dstparent
            |                |
            |               dst
            port
        */

        /* find portparent cousin above dst */
        dstparent = dst;
        while (dstparent->depth != portparent->depth)
            dstparent = dstparent->parent;
        if (dstparent != portparent) {
            /* go horizontal from portparent to dstparent if different */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_HORIZONTAL;
            edges[i].hwloc.src_obj = portparent;
            edges[i].hwloc.dest_obj = dstparent;
            /* find the weight through common ancestor */
            ancestor = hwloc_get_common_ancestor_obj(topology, portparent, dstparent);
            edges[i].hwloc.weight = (portparent->depth - ancestor->depth) * 2 - 1;
            i++;
        }
        if (dstparent != dst && (flags & NETLOC_MAP_PATHS_FLAG_VERTICAL)) {
            /* go down from dstparent to dst */
            edges[i].type = NETLOC_MAP_EDGE_TYPE_HWLOC_CHILD;
            edges[i].hwloc.src_obj = dstparent;
            edges[i].hwloc.dest_obj = dst;
            edges[i].hwloc.weight = dst->depth - dstparent->depth;
            i++;
        }
    }

    *edgesp = edges;
    *nr_edgesp = i;
    return 0;
}

int netloc_map_paths_build(netloc_map_t _map,
                           hwloc_topology_t srctopo, hwloc_obj_t srcobj,
                           hwloc_topology_t dsttopo, hwloc_obj_t dstobj,
                           unsigned long flags,
                           netloc_map_paths_t *_paths, unsigned *nr)
{
    struct netloc_map *map = _map;
    struct netloc_map__paths *paths;
    struct netloc_map__server *srcserver, *dstserver;
    unsigned max,i,j;
    int err;

    /* check flags */
    if (flags & ~(NETLOC_MAP_PATHS_FLAG_IO
                  |NETLOC_MAP_PATHS_FLAG_VERTICAL))
        return -1;

    /* don't let special objects be used, they would mess up the weights */
    if (srcobj->depth == (unsigned) HWLOC_TYPE_DEPTH_UNKNOWN
        || dstobj->depth == (unsigned) HWLOC_TYPE_DEPTH_UNKNOWN)
        return -1;

    srcserver = netloc_map__get_server_by_topology(map, srctopo);
    if (!srcserver)
        return -1;
    dstserver = netloc_map__get_server_by_topology(map, dsttopo);
    if (!dstserver)
        return -1;

    /* preallocate the maximal number of paths */
    max = 0;
    for(i=0; i<srcserver->nr_ports; i++) {
        struct netloc_map__port *srcport = srcserver->ports[i];
        for(j=0; j<dstserver->nr_ports; j++) {
            struct netloc_map__port *dstport = dstserver->ports[j];
            if (srcport->subnet == dstport->subnet)
                max++;
        }
    }

    paths = calloc(1, sizeof(*paths));
    if (!paths)
        return -1;
    paths->map = map;
    paths->flags = flags;

    paths->nr_paths = 0;
    paths->paths = calloc(max, sizeof(*paths->paths));
    if (!paths->paths) {
        free(paths);
        return -1;
    }

    for(i=0; i<srcserver->nr_ports; i++) {
        struct netloc_map__port *srcport = srcserver->ports[i];
        netloc_topology_t netloc = srcport->subnet->topology;
        netloc_node_t *srcnode;
        srcnode = netloc_get_node_by_physical_id(netloc, srcport->id);
        assert(srcnode);

        for(j=0; j<dstserver->nr_ports; j++) {
            struct netloc_map__port *dstport = dstserver->ports[j];

            if (srcport->subnet != dstport->subnet)
                continue;

            netloc_node_t *dstnode;
            dstnode = netloc_get_node_by_physical_id(netloc, dstport->id);
            assert(dstnode);

            struct netloc_map__path *path = &paths->paths[paths->nr_paths];
            path->edges = NULL;
            path->nr_edges = 0;

            /* hwloc edges in the source node */
            err = netloc_map__paths_build_hwloc_edge_to_port(srctopo,
                                                             srcobj, srcport->hwloc_obj,
                                                             flags,
                                                             &path->edges, &path->nr_edges);
            if (err < 0)
                continue;

            /* netloc edges */
            netloc_edge_t **nedges = NULL;
            int nr_nedges = 0;
            int res = netloc_get_path(netloc, srcnode, dstnode, &nr_nedges, &nedges, 0);
            if (NETLOC_SUCCESS != res)
                continue;
            path->edges = realloc(path->edges, (path->nr_edges + nr_nedges) * sizeof(*path->edges));
            if (!path->edges)
                goto out;
            unsigned k;
            for(k=0; k<(unsigned) nr_nedges; k++) {
                struct netloc_map_edge_s *edge = &path->edges[path->nr_edges++];
                edge->type = NETLOC_MAP_EDGE_TYPE_NETLOC;
                edge->netloc.topology = netloc;
                edge->netloc.edge = nedges[k];
            }

            /* hwloc edges in the destination node */
            err = netloc_map__paths_build_hwloc_edge_from_port(dsttopo,
                                                               dstport->hwloc_obj, dstobj,
                                                               flags,
                                                               &path->edges, &path->nr_edges);
            if (err < 0)
                continue;

            paths->nr_paths++;
        }
    }

 out:
    *_paths = paths;
    *nr = paths->nr_paths;
    return 0;
}

int netloc_map_paths_get(netloc_map_paths_t _paths, unsigned idx,
                         struct netloc_map_edge_s **edges, unsigned *nr_edges)
{
    struct netloc_map__paths *paths = _paths;

    if (idx >= paths->nr_paths) {
        errno = EINVAL;
        return -1;
    }

    *edges = paths->paths[idx].edges;
    *nr_edges = paths->paths[idx].nr_edges;
    return 0;
}

int netloc_map_paths_destroy(netloc_map_paths_t _paths)
{
    struct netloc_map__paths *paths = _paths;
    unsigned i;
    for(i=0; i<paths->nr_paths; i++)
        free(paths->paths[i].edges);
    free(paths->paths);
    free(paths);
    return 0;
}

/******************
 * Debug
 */

int
netloc_map_find_neighbors(netloc_map_t _map,
                          const char *hostname, unsigned maxdepth)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;
    unsigned i,j,k,l;

    if (!map->merged) {
        errno = EINVAL;
        return -1;
    }

    server = netloc_map__get_server_by_name(map, hostname);
    if (!server)
        return -1;

    unsigned done_nr_allocated = 32;
    const char **done_ids = malloc(done_nr_allocated * sizeof(*done_ids));
    unsigned prev_nr_allocated = 32;
    const char **prev_ids = malloc(prev_nr_allocated * sizeof(*prev_ids));
    unsigned next_nr_allocated = 32;
    const char **next_ids = malloc(next_nr_allocated * sizeof(*next_ids));

    for(i=0; i<server->nr_ports; i++) {
        struct netloc_map__port *port = server->ports[i];
        struct netloc_map__subnet *subnet = port->subnet;
        netloc_topology_t netloc = subnet->topology;
        unsigned depth;

        assert(netloc);

        unsigned done_nr = 1;
        done_ids[0] = port->id;
        unsigned prev_nr = 1;
        prev_ids[0] = port->id;
        unsigned next_nr = 0;

        printf("Starting from port %s in subnet type %d id %s\n",
               port->id, subnet->type, subnet->id);

        for(depth = 1; depth <= maxdepth; depth++) {
            printf("Looking at distance %u in subnet type %d id %s\n",
                   depth, subnet->type, subnet->id);

            for(j=0; j<prev_nr; j++) {
                netloc_edge_t **edges = NULL;
                int nredges = 0;
                netloc_node_t *node = netloc_get_node_by_physical_id(netloc, prev_ids[j]);
                if (!node) {
                    fprintf(stderr, "lookup_table_access(nodes) failed, port down or unknown?\n");
                    continue;
                }

                netloc_get_all_edges(netloc, node, &nredges, &edges);

                for(k=0; k<(unsigned)nredges; k++) {
                    netloc_edge_t *edge = edges[k];
                    const char* id = edge->dest_node_id;
                    int done = 0;
                    for(l=0; l<done_nr; l++) {
                        if (!strcmp(id, done_ids[l])) {
                            done = 1;
                            break;
                        }
                    }
                    if (done)
                        continue;

                    if (edge->dest_node_type == NETLOC_NODE_TYPE_SWITCH) {
                        printf("Queueing switch %s\n", id);
                        if (next_nr == next_nr_allocated) {
                            //next_nr_allocated = next_nr_allocated;
                            next_ids = realloc(next_ids, next_nr_allocated * sizeof(*next_ids));
                            if (!next_ids)
                                goto out;
                        }
                        next_ids[next_nr++] = id;

                    } else {
                        const char *name = "unknown";
                        struct netloc_map__port *port = netloc_map__get_port_by_id(subnet, id);
                        if (port)
                            name = port->server->name;
                        printf("Found server %s port %s\n", name, id);
                    }

                    if (done_nr == done_nr_allocated) {
                        done_nr_allocated <<= 1;
                        done_ids = realloc(done_ids, done_nr_allocated * sizeof(*done_ids));
                        if (!done_ids)
                            goto out;
                    }
                    done_ids[done_nr++] = id;
                }
            }

            if (!next_nr)
                break;

            if (next_nr > prev_nr_allocated) {
                prev_nr_allocated = next_nr_allocated;
                prev_ids = realloc(prev_ids, prev_nr_allocated * sizeof(*prev_ids));
                if (!prev_ids)
                    goto out;
            }
            memcpy(prev_ids, next_ids, next_nr * sizeof(*prev_ids));
            prev_nr = next_nr;
            next_nr = 0;
        }
    }

 out:
    free(done_ids);
    free(prev_ids);
    free(next_ids);
    return 0;
}

int
netloc_map_dump(netloc_map_t _map)
{
    struct netloc_map *map = _map;
    struct netloc_map__server *server;
    struct netloc_map__subnet *subnet;
    const char * tmp_str = NULL;

    printf("\n");
    printf("==== Dump of netloc map state ====\n");

    printf("Subnets:\n");
    subnet = map->subnet_first;
    while (subnet) {
        tmp_str = netloc_decode_network_type_readable(subnet->type);
        printf(" %s - %s\n", tmp_str, subnet->id );
        subnet = subnet->next;
        tmp_str = NULL;
    }

    printf("Servers:\n");
    server = map->server_first;
    while (server) {
        unsigned i;
        printf(" %s (%d port(s))\n", server->name, server->nr_ports);
        for(i=0; i<server->nr_ports; i++) {
            struct netloc_map__port *port = server->ports[i];
            printf("  port %s in %s subnet %s\n", port->id, netloc_decode_network_type_readable(port->subnet->type), port->subnet->id);
        }
        server = server->next;
    }

    printf("==== End of dump of netloc map state ====\n");
    printf("\n");

    return 0;
}
