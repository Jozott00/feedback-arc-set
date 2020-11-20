#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#define SHM_NAME "/graphresult"
#define MAX_DATA (50)
#define PROG_NAME ""
#define USED_SEM "/fb_arc_set_used_sem"
#define FREE_SEM "/fb_arc_set_free_sem"

extern struct myshm
{
    unsigned int state;
    unsigned int data[50];
} * myshm;

extern sem_t *used_sem;
extern sem_t *free_sem;

void error_msg(char *program, int line, char *msg);

void error_exit(char *program, int line, char *msg);

void success_exit(char *program);

void clean_exit();