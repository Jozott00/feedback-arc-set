#ifndef CB_H
#define CB_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#define SHM_NAME "/graphresult"
#define BUFFER_SIZE (50)
#define EDGE_COUNT (8)
#define USED_SEM "/fb_arc_set_used_sem"
#define FREE_SEM "/fb_arc_set_free_sem"
#define MUTEX_SEM "/fb_arc_set_mutex_sem"

/* Logical setup for shm and sem */

void error_msg(char *program, int line, char *msg, int with_errno);

void error_exit(char *program, int line, char *msg, int with_errno);

void success_exit(char *program);

void clean_up(char *progn);

void setup_supervisor(void);

void setup_generator(void);

void print_semaphores(void);

// global structs and variables

typedef struct
{
    unsigned int a;
    unsigned int b;
} edge;

typedef struct
{
    int size;
    edge edges[EDGE_COUNT]; //only solution with max of 8 edges can be stored
} arcset;

struct graph_shm
{
    int wr_pos;
    unsigned int status;
    arcset sets[BUFFER_SIZE];
} * shm;

sem_t *used_sem;
sem_t *free_sem;
sem_t *mutex_sem;

/* Actual Buffer Functions*/

int write_set(arcset set);

int read_delete_set(arcset *set);

int set_status(int status);

unsigned int get_status(void);

void print_solution(const char *prog, arcset *set);

#endif //CB_H
