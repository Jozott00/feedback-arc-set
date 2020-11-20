#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "global.h"

char *program_name = "global.c";

void error_msg(char *program, int line, char *msg)
{
    fprintf(stderr, "[%s:%d] ERROR: %s: %s\n", program, line, msg, strerror(errno));
}

void error_exit(char *program, int line, char *msg)
{
    error_msg(program, line, msg);
    clean_up(program);
    exit(EXIT_FAILURE);
}

void success_exit(char *program)
{
    clean_up(program);
    exit(EXIT_SUCCESS);
}

void clean_up(char *progn)
{

    if (munmap(myshm, sizeof(myshm)) == -1)
        error_exit(program_name, __LINE__, "Could not close mapping");

    if (sem_close(free_sem) == -1 || sem_close(used_sem) == -1)
        error_msg(program_name, __LINE__, "At least one semaphore could not be closed");

    if (progn == "supervisor.c")
    {
        if (shm_unlink(SHM_NAME) == -1)
            error_msg(program_name, __LINE__, "Could not unlink shared memory");

        if (sem_unlink(FREE_SEM) == -1 || sem_unlink(USED_SEM) == -1)
            error_msg(program_name, __LINE__, "At least one semaphore could not be unlinked");
    }
}