#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>

#include "circularBuffer.h"

char *sup_name = "supervisor.c";
extern volatile __sig_atomic_t quit;

int main(int argc, char const *argv[])
{

    setup_supervisor();

    print_semaphores();

    arcset best_set;
    best_set.size = __INT16_MAX__;

    arcset *set = malloc(sizeof(arcset));

    // size_t debug = 0;
    while (quit != 1)
    {

        // if (debug == 9999 || debug == 999999 || debug == 99999999)
        //     print_semaphores();
        // debug++;
        // // printf("\nINFO: reading... \n\n");

        if (read_delete_set(set) == -1)
            continue;

        if (set == NULL)
            error_exit(sup_name, __LINE__, "set is NULL", 0);

        if (set->size < best_set.size)
        {
            best_set = *set;
            print_solution(argv[0], &best_set);
        }

        if (best_set.size <= 0)
        {
            printf("This graph is asyclic! \n");
            quit = 1;
        }

        // sleep(2);
    }

    set_status(1);
    success_exit(sup_name);

    return 0;
}
