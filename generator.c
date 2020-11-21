/**
 * @project: Feedback Arc Set
 * @module generator
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 19.11.2020
 * @section File Overview
 * generator is responsible for generating feedback arc sets by creating random permutations of 
 * all nodes. After generating it, the arc set is written to shared memory via write_set() by circularBuffer.c
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include "circularBuffer.h"

static const char *gen_name = "generator.c"; /**< global name of the program file (set for erro messages). */
extern volatile sig_atomic_t quit;           /**< is set extern(in circularBuffer.c) and indicates if process should end. */

/**
 * @brief Generates random permutation of node array
 * @details The function generates the permutation with the Fisher-Yates algorithm.
 * First the set of nodes is generated. Because we have the maximum node and we know 
 * that the nodes are name from 0 to n we can generate the set by simply iterating
 * from 0 to maxnode. 
 * 
 * The Fisher-Yates algorithm runs from the last node to the first node and
 * calculates a random index of the set. The current Node-value need to be stored in
 * a tmp variable, then on our current position comes the value of the index we just calculated randomly.
 * Since the set now contains the value of the index we calculated twice and the old value of
 * the current index not a single time, we need to set to the calculated index our tmp variable
 * with the old value of the current index.
 * The algorithm does it for each element from the tail of the array to the head.
 * 
 * @param max_node Is the maximum value of all nodes and implict the length of the set - 1.
 * @return It return the pointer to the first value of the set.
 * 
 * @warning Not a warning, but a inefficency issue. If for each geneartion of the permutation the old permutation would
 * be used instead of generating always the new node set, the perfomance would increase.
 * 
 */
static int *get_perm(size_t max_node)
{
    size_t tmps = max_node + 1; /* maxnode + 1 because max_node is the larges node of graph but 0 is also a node */
    int *perm = malloc(sizeof(int) * tmps + 1);
    for (int i = 0; i < tmps; i++)
        perm[i] = i;

    for (int i = tmps - 1; i >= 0; --i) /* Fisher-Yates algorithm */
    {
        int j = rand() % (i + 1);

        int temp = perm[i];
        perm[i] = perm[j];
        perm[j] = temp;
    }
    return perm;
}

/**
 * @brief Generates a new feedback arc set.
 * 
 * @details The function generates an arc set by getting a random permutation of
 * nodes from get_perm() and comparing it with the edges of the given graph.
 * 
 * It runs through all edges of the graph. For each edge it runs throug
 * the whole permutation of nodes and searches for wether node a or b of the edge.
 * When it found on node it can stop because:
 *      - if the node is the first node this edge is not in the arc set. (because the second is "greater" than the first)
 *      - if the node is the second node this edge is in the arc set for sure. (because the first is "greater than the second")
 * If the edge should be in the arc set, it is added to the index of add_i and increments add_i by 1.
 * It takes the next edge and does the same process again.
 * 
 * At the end we got an array of edges in our arc set. add_i is added else size of the arc set and
 * 0 is returned.
 * 
 * If the add_i index reaches 7 so there are would be more than 8 edges, the function returns -1, so
 * the generator know that this arcset should not be written to shared memory.
 * 
 * @see get_perm()
 * 
 * @param set Is a pointer to the arc set where the generated arc set should be stored
 * @param len Is the length of the given graph (number of edges)
 * @param graph Is a pointer to the graph the function is operating with
 * @param max_node is the maximum value of all nodes an implicit indicates the length of the nodes array
 * 
 * @return Return 0 if successfully stored arcset at pointer location and -1 if the arcset
 * would have more than 8 edges.
 * 
 * 
*/
static int gen_set(arcset *set, size_t len, edge *graph, size_t max_node)
{
    int *perm = get_perm(max_node);

    size_t add_i = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (add_i >= EDGE_COUNT) /* store max of 8 edges... max index 7 */
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

/**
 * @brief Generates graph from given program arguments
 * 
 * @details The arguments of the generator progam looks like "generator 1-2 2-3 3-1" where
 * each "*-*" indicates an edge of the graph. The * are the values of this edge.
 * The function runs through alle strings of the given argument values and 
 * parse them to and edge type. This is done by using sscanf() where the
 * format of the string can be set. sscanf() does not have an error code when the format
 * does match with the string, but it returns the number of found parameter.
 * 
 * If this number is not 2 (because 1 edge has 2 nodes) the inbut is not valid -> an
 * error exit is executing
 * 
 * If not, the edge is stored in the graph at its index i.
 * The variable mn represents the maximum node. If one of the new obtained nodes 
 * is greater than the old maximum node, the node updates. 
 * 
 * @param graph pointer to the location where the graph should be stored
 * @param len pointer to length of the argument value array
 * @param argv pointer to string array with all positional arguments
 * 
 * @return Returns a positiv long which represents the maximum node found in the graph.
 * 
*/
static size_t create_graph(edge *graph, size_t *len, char const *argv[])
{
    int v1 = 0;
    int v2 = 0;
    size_t mn = 0;

    for (size_t i = 1; i < *len + 1; i++)
    {
        int readitems = sscanf(argv[i], "%d-%d", &v1, &v2);
        if (readitems != 2 || v1 < 0 || v2 < 0)
            error_exit((char *)gen_name, __LINE__ - 2, "Input is not a graph!", 0);
        edge e = {.a = v1,
                  .b = v2};

        mn = v1 > mn ? (size_t)v1 : mn;
        mn = v2 > mn ? (size_t)v2 : mn;

        graph[i - 1] = e;
    }

    return mn;
}

/**
 * @brief Managing whole process of generator
 * 
 * @details First the graph is created via create_graph(). If no exception is thrown, all
 * semaphores and shm will get setted up by setup_generator.
 * Now the while loop is entered and the status of the shm is asked. 
 * Then a new arc set is generated and written two shared memory via 
 * write_set() from circularBuffer.c. Except gen_set() return -1, than
 * this solution will ignored and the process jumps directly to the next loop sequence.
 * 
 * If the atomic variable quit equals 1, the loop will break an succes exit will executed.
 * 
 * @see create_graph()
 * @see gen_set()
 * 
 * @param argc If the count of arguments is less than 2 an exception is thrown.
 * @param argv Pointer to positional arguments. Must have structure [*-*] with numbers at each end of the "-".
 * 
*/
int main(int argc, char const *argv[])
{

    if (argc < 2)
        error_exit((char *)gen_name, __LINE__ - 1, "No arguments passed!", 0);

    size_t len = argc - 1;
    edge *graph = (edge *)malloc(len * sizeof(edge));
    size_t maxNode = create_graph(graph, &len, argv);

    setup_generator();

    while (quit != 1)
    {
        if (get_status() == 1)
            break;

        arcset *set = malloc(sizeof(arcset));
        if (gen_set(set, len, graph, maxNode) == -1)
            continue;

        write_set(*set);
    }

    printf("\nDanke und auf Wiedersehen!\n\n");
    success_exit((char *)gen_name);

    return 0;
}
