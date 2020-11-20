#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>

#include "circularBuffer.h"

static char *cb_name = "circularBuffer.c";
volatile sig_atomic_t quit = 0;

static bool usem_set = false;
static bool fsem_set = false;
static bool msem_set = false;

void error_msg(char *program, int line, char *msg, int with_errno)
{
    if (with_errno == 1)
        fprintf(stderr, "[%s:%d] ERROR: %s: %s\n", program, line, msg, strerror(errno));
    else
        fprintf(stderr, "[%s:%d] ERROR: %s\n", program, line, msg);
}

void error_exit(char *program, int line, char *msg, int with_errno)
{
    error_msg(program, line, msg, with_errno);
    clean_up(program);
    exit(EXIT_FAILURE);
}

void success_exit(char *program)
{
    clean_up(program);
    exit(EXIT_SUCCESS);
}

void handle_seg_signal(int signal)
{
    error_msg(cb_name, __LINE__, "Segementation fault! Fix that!", 0);
    clean_up("supervisor.c");
    _exit(1);
}

void handle_signal(int signal)
{
    if (signal == SIGSEGV)
        handle_seg_signal(signal);
    else
        quit = 1;
}

void setup_signal(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
}

void setup_supervisor(void)
{

    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1)
        error_exit(cb_name, __LINE__, "Could not open shared memory", 1);

    if (ftruncate(shmfd, sizeof(*shm)) < 0)
        error_exit(cb_name, __LINE__, "Shared memory could not be assigned a memory size", 1);

    shm = mmap(NULL, sizeof(*shm), PROT_WRITE | PROT_READ, MAP_SHARED, shmfd, 0); // Linux uses MAP_ANONYMOUS !!!
    if (shm == MAP_FAILED)
        error_exit(cb_name, __LINE__, "Mapping of shared memory failed", 1);

    if (close(shmfd) == -1)
        error_msg(cb_name, __LINE__, "File descriptor cannot be closed", 1);

    used_sem = sem_open(USED_SEM, O_CREAT, 0600, 0);
    usem_set = true;
    free_sem = sem_open(FREE_SEM, O_CREAT, 0600, BUFFER_SIZE);
    fsem_set = true;
    mutex_sem = sem_open(MUTEX_SEM, O_CREAT, 0600, 1);
    msem_set = true;

    if (used_sem == SEM_FAILED || free_sem == SEM_FAILED || mutex_sem == SEM_FAILED)
        error_exit(cb_name, __LINE__, "Could not open semaphore", 1);

    setup_signal();
}

void setup_generator(void)
{

    int shmfd = shm_open(SHM_NAME, O_RDWR, 0600);
    if (shmfd == -1)
        error_exit(cb_name, __LINE__, "Could not open shared memory", 1);

    shm = mmap(NULL, sizeof(*shm), PROT_WRITE | PROT_READ, MAP_SHARED, shmfd, 0);

    used_sem = sem_open(USED_SEM, O_CREAT);
    usem_set = true;
    free_sem = sem_open(FREE_SEM, O_CREAT);
    fsem_set = true;
    mutex_sem = sem_open(MUTEX_SEM, O_CREAT);
    msem_set = true;

    setup_signal();
}

void print_semaphores(void)
{
    int vUsed;
    int vFree;
    sem_getvalue(used_sem, &vUsed);
    sem_getvalue(free_sem, &vFree);
    printf("UsedSEM = %d\n", vUsed);
    printf("FreeSEM = %d\n", vFree);
    if (msem_set)
    {
        int vMutex;
        sem_getvalue(mutex_sem, &vMutex);
        printf("MutexSEM = %d\n", vMutex);
    }
}

void print_solution(const char *prog, arcset *set)
{
    printf("[%s] Solution with %d edges: ", prog, set->size);
    for (int i = 0; i < set->size; i++)
    {
        printf(" %d-%d", set->edges[i].a, set->edges[i].b);
    }
    printf("\n");
}

void clean_up(char *progn)
{

    printf("INFO: cleaning up shm and sem...\n\n");

    if (munmap(shm, sizeof(shm)) == -1)
        error_exit(cb_name, __LINE__, "Could not close mapping", 1);

    if (usem_set || fsem_set || msem_set)
        if (sem_close(free_sem) == -1 || sem_close(used_sem) == -1 || sem_close(mutex_sem) == -1)
            error_msg(cb_name, __LINE__, "At least one semaphore could not be closed", 1);

    if (strcmp(progn, "supervisor.c") == 0)
    {
        if (shm_unlink(SHM_NAME) == -1)
            error_msg(cb_name, __LINE__, "Could not unlink shared memory", 1);

        if (sem_unlink(FREE_SEM) == -1 || sem_unlink(USED_SEM) == -1)
            error_msg(cb_name, __LINE__, "At least one semaphore could not be unlinked", 1);

        if (sem_unlink(MUTEX_SEM) == -1)
            error_msg(cb_name, __LINE__, "At least one semaphore could not be unlinked", 1);
    }
}

int write_set(arcset set)
{

    if (sem_wait(free_sem) == -1)
    {
        if (errno != EINTR)
            error_exit(cb_name, __LINE__, "Semaphore equals -1 but it shouldn't", 1);
        else
            return -1;
    }

    if (sem_wait(mutex_sem) == -1)
    {
        if (errno != EINTR)
            error_exit(cb_name, __LINE__, "Semaphore equals -1 but it shouldn't", 1);
        else
            return -1;
    }

    shm->sets[shm->wr_pos] = set;
    shm->wr_pos = (shm->wr_pos + 1) % BUFFER_SIZE;

    sem_post(mutex_sem);

    sem_post(used_sem);

    return 0;
}

//returns struct because we want to set NULL if failed to read.
int read_delete_set(arcset *set)
{

    static int rd_pos = 0;

    if (sem_wait(used_sem) == -1)
    {
        if (errno != EINTR)
            error_exit(cb_name, __LINE__, "Semaphore equals -1 but it shouldn't (reading)", 1);
        else
            return -1;
    }

    *set = shm->sets[rd_pos];
    sem_post(free_sem);

    rd_pos++;
    rd_pos %= BUFFER_SIZE;

    return 0;
}

int set_status(int state)
{
    shm->status = state;

    return 0;
}

unsigned int get_status()
{
    return shm->status;
}