#include <stdio.h>
#include <mpi.h>
#include <hwloc.h>
#include <private/netloc.h>

typedef struct {
    UT_hash_handle hh; /* Makes this structure hashable */
    char *name; /* Hash key */
    UT_array *slots;
    UT_array *ranks;
} node_t;

int main(int argc, char **argv)
{
    int rank;
    int num_ranks;
    MPI_Status status;
    hwloc_topology_t topology;
    hwloc_cpuset_t set;
    int pu_rank = -1;
    char name[1024];
    int resultlen;

    node_t *nodes = NULL;

    MPI_Init(&argc,&argv);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
        exit(1);
    }

    FILE *output = fopen(argv[1], "w");

    if (!output) {
        perror("fopen");
        exit(2);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    hwloc_topology_init(&topology);
    hwloc_topology_load(topology);
    set = hwloc_bitmap_alloc();
    hwloc_get_cpubind(topology, set, 0);
    pu_rank = hwloc_bitmap_first(set);

    MPI_Get_processor_name(name, &resultlen);
    resultlen++;

    if (rank == 0) {
        node_t *node;

        /* Rank 0 info */
        /* Find node */
        HASH_FIND_PTR(nodes, &name, node);
        /* If node does not exist yet, create it */
        if (!node) {
            node = (node_t *)malloc(sizeof(node_t));
            node->name = name;
            utarray_new(node->slots, &ut_int_icd);
            utarray_new(node->ranks, &ut_int_icd);
            HASH_ADD_KEYPTR(hh, nodes, node->name, strlen(node->name), node);
        }
        /* Add the slot to the list of slots */
        utarray_push_back(node->slots, &pu_rank);
        utarray_push_back(node->ranks, &rank);

        /* Info about other ranks */
        for (int p = 1; p < num_ranks; p++) {
            /* Receive node name size, and slot index */
            char *nodename;
            int buffer[2];
            MPI_Recv (buffer, 2, MPI_INT, p, 0, MPI_COMM_WORLD, &status);
            int size = buffer[0];
            int slot = buffer[1];

            /* Receive node name */
            nodename = (char *)malloc(sizeof(char[size]));
            MPI_Recv(nodename, size, MPI_CHAR, p, 0, MPI_COMM_WORLD, &status);

            /* Find node */
            HASH_FIND_STR(nodes, nodename, node);
            /* If node does not exist yet, create it */
            if (!node) {
                node = (node_t *)malloc(sizeof(node_t));
                node->name = nodename;
                utarray_new(node->slots, &ut_int_icd);
                utarray_new(node->ranks, &ut_int_icd);
                HASH_ADD_KEYPTR(hh, nodes, node->name, strlen(node->name), node);
            }
            /* Add the slot to the list of slots */
            utarray_push_back(node->slots, &slot);
            utarray_push_back(node->ranks, &p);
        }

        /* Write the list of nodes and slots by node */

        /* Number of nodes */
        int num_nodes = HASH_COUNT(nodes);
        fprintf(output, "%d", num_nodes);

        /* Names of nodes */
        node_t *node_tmp;
        HASH_ITER(hh, nodes, node, node_tmp) {
            fprintf(output, " %s", node->name);
        }

        /* Number of slots by node */
        HASH_ITER(hh, nodes, node, node_tmp) {
            int num_slots = utarray_len(node->slots);
            fprintf(output, " %d", num_slots);
        }

        /* List of slots */
        HASH_ITER(hh, nodes, node, node_tmp) {
            int num_slots = utarray_len(node->slots);
            int *slots = (int *)node->slots->d;
            int *ranks = (int *)node->ranks->d;
            for (int s = 0; s < num_slots; s++) {
                fprintf(output, " %d", slots[s]);
                fprintf(output, " %d", ranks[s]);
            }
        }
    } else {
        int buffer[2];
        buffer[0] = resultlen;
        buffer[1] = pu_rank;
        /* Send node name size, and slot index */
        MPI_Send(buffer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
        /* Send node name */
        MPI_Send(name, resultlen, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}
