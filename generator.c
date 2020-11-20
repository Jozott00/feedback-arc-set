#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>

#include "circularBuffer.h"

static char *gen_name = "generator.c";
extern volatile __sig_atomic_t quit; // be careful, if one generator dies, maybe each process with quit var dies.

int *get_perm(size_t max_node)
{
    // because 0 to max_node
    size_t tmps = max_node + 1;
    int *perm = malloc(sizeof(int) * tmps + 1);
    for (int i = 0; i < tmps + 1; i++)
        perm[i] = i;

    for (int i = tmps - 1; i >= 0; --i)
    {
        int j = rand() % (i + 1);

        int temp = perm[i];
        perm[i] = perm[j];
        perm[j] = temp;
    }

    // printf("Permutation: {");
    // for (int i = 0; i < tmps; i++)
    // {
    //     printf(" %d,", perm[i]);
    // }
    // printf("}\n");

    return perm;
}

int gen_set(arcset *set, size_t len, edge *graph, size_t max_node)
{
    int *perm = get_perm(max_node);

    size_t add_i = 0;
    for (size_t i = 0; i < len; i++)
    {
        //store max of 8 edges... max index 7
        if (add_i >= EDGE_COUNT)
            return -1;

        size_t a = (graph + i)->a;
        size_t b = (graph + i)->b;

        bool add = false;
        for (size_t j = 0; j < max_node + 1; j++)
        {
            if (perm[j] == a)
                break;
            if (perm[j] == b)
            {
                add = true;
                break;
            }
        }

        if (add)
        {
            set->edges[add_i] = graph[i];
            add_i++;
        }
    }

    set->size = add_i;

    return 0;
}

size_t create_graph(edge *graph, size_t *len, char const *argv[])
{
    int v1 = 0;
    int v2 = 0;
    size_t mn = 0;
    // printf("Graph: { ");
    for (size_t i = 1; i < *len + 1; i++)
    {
        int readitems = sscanf(argv[i], "%d-%d", &v1, &v2);
        if (readitems != 2 || v1 < 0 || v2 < 0)
            error_exit(gen_name, __LINE__ - 2, "Input is not a graph!", 0);
        edge e = {.a = v1,
                  .b = v2};

        mn = v1 > mn ? (size_t)v1 : mn;
        mn = v2 > mn ? (size_t)v2 : mn;

        // printf(" %d->%d,", e.a, e.b);
        graph[i - 1] = e;
    }
    // printf(" }\n");
    // printf("MAXvalue: %ld\n", mn);

    return mn;
}

void print_graph(edge *graph, int len)
{

    printf("\nGraph from struct: \n");
    printf("\nLENGTH: %d\n", len);
    for (size_t i = 0; i < len; i++)
    {
        printf("Edge %ld: %d->%d\n", i, graph[i].a, graph[i].b);
    }
}

int main(int argc, char const *argv[])
{

    if (argc < 2)
        error_exit(gen_name, __LINE__ - 1, "No arguments passed!", 0);

    size_t len = argc - 1;
    edge *graph = (edge *)malloc(len * sizeof(edge));
    size_t maxNode = create_graph(graph, &len, argv);

    // print_graph(graph, len);

    setup_generator();

    // print_semaphores();

    // size_t debug = 0;
    while (quit != 1)
    {
        if (get_status() == 1)
            break;

        // if (debug == 100 || debug == 999999 || debug == 99999999)
        //     print_semaphores();

        arcset *set = malloc(sizeof(arcset));
        if (gen_set(set, len, graph, maxNode) == -1)
            continue;

        // printf("INFO: Edgecount: %d\n", set->size);
        // for (int i = 0; i < set->size; i++)
        // {
        //     edge e = set->edges[i];
        //     printf("INFO: Edge %d: %d->%d\n", i, e.a, e.b);
        // }

        write_set(*set);
        // print_semaphores();
        // sleep(1);
    }

    printf("\nDanke und Aufwiedersehen!\n\n");
    // print_semaphores();
    success_exit((char *)gen_name);

    return 0;
}
