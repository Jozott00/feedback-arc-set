/**
 * @project: Feedback Arc Set
 * @module supervisor
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 19.11.2020
 * @section File Overview
 * supervisor.c is responsible for the validation of new incoming arcsets
 * to determine if they are better than the previous one and if yes, is the graph of the
 * arcset asyclic? It is also the only output of new edges.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#include "circularBuffer.h"

static const char *sup_name = "supervisor.c"; /**< global name of the program file (set for erro messages). */
extern volatile __sig_atomic_t quit;          /**< is set extern(in circularBuffer.c) and indicates if process should end. */

/**
 * @brief Runs the process of supervisor
 * 
 * @details First the semaphores and shm will set up (managed by circularBuffer.c).
 * In addition the best solution set is declared and the size of it is set to maximum Interger 
 * so each set is better then the initialized best arcset.
 */
int main(int argc, char const *argv[])
{

    setup_supervisor(); /* setup for sems and shm */

    arcset best_set;
    best_set.size = __INT16_MAX__;

    arcset *set = malloc(sizeof(arcset));
    while (quit != 1)
    {

        if (read_delete_set(set) == -1) /* if interrupted by signal break loop*/
            continue;

        if (set == NULL)
            error_exit((char *)sup_name, __LINE__, "set is NULL", 0);

        if (set->size < best_set.size)
        {
            best_set = *set;
            print_solution(argv[0], &best_set);
        }

        if (best_set.size <= 0) /* if set has 0 edges than the graph is asyclic -> break out while */
        {
            printf("This graph is asyclic! \n");
            quit = 1;
        }
    }

    set_status(1);                  /* set status to 1 so all generate know that process is ended */
    success_exit((char *)sup_name); /* exit with success and clean up before leaving */

    return 0;
}
