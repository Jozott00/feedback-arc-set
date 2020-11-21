/**
 * @project: Feedback Arc Set
 * @module circularBuffer
 * @author Johannes Zottele 11911133
 * @version 1.0
 * @date 19.11.2020
 * @section File Overview
 * circularBuffer is responsible for the whole circular buffer management, including semaphores and
 * shared memory. It contains also some general functions, such as error printing and exiting.
 * It also takse care of cleaning up all semaphores and shared memory.
 */

#ifndef CB_H
#define CB_H

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>

#define SHM_NAME "/graphresult"           /**< name for shm file */
#define BUFFER_SIZE (50)                  /**< length of buffercircular */
#define EDGE_COUNT (8)                    /**< length of stored edges in arcset */
#define USED_SEM "/fb_arc_set_used_sem"   /**< name of semaphore for used space of buffer */
#define FREE_SEM "/fb_arc_set_free_sem"   /**< name of semaphore for free space of buffer */
#define MUTEX_SEM "/fb_arc_set_mutex_sem" /**< name of semaphore for exklusive write permission for process to buffer */

/*************************************
 *  GENERAL GLOBALLY USED FUNCTIONS  *
 *************************************/
/**
 * @brief Prints error message to stderr.
 * 
 * @details Prints "[programm:line] ERROR " followed by given message. 
 * In case that with_errno is set to 1, strerror() function is used to get
 * errormessage to actual errno number. Should only be used, if it is
 * known that errno message was set for this error. The whole message
 * is printed out to stderr.
 * 
 * @see strerror()
 * 
 * @param program string of program for identity actual program.
 * @param line int to print in which line the error appears.
 * @param msg text to print. Should only describe when error happens, not why.
 * @param with_errno if is set to 1 errno number will be used. Otherwise only custom error is printed. 
 */
void error_msg(char *program, int line, char *msg, int with_errno);

/**
 * @brief Manages error exit of program.
 * 
 * @details It calls erro_msg() as well as clean_up(). Last but not least it uses
 * exit() to exit with EXIT_FAILURE status.
 * 
 * @see err_msg()
 * @see clean_up()
 * 
 * @param program string of program for identity actual program.
 * @param line int to print in which line the error appears.
 * @param msg text to print. Should only describe when error happens, not why.
 * @param with_errno if is set to 1 errno number will be used. Otherwise only custom error is printed. 
 */
void error_exit(char *program, int line, char *msg, int with_errno);

/**
 * @brief Manages success exit of program.
 * 
 * @details It calls clean_up() and exit() with EXIT_SUCCESS status.
 * 
 * @see clean_up()
 * 
 * @param program program which is ending. is needed to manage the clean_up() function.
 */
void success_exit(char *program);

/*******************************************
 *  BUFFER SPECIFIC VARIABLES AND STRUCTS  *
 *******************************************/

/**
 * @brief Defines new type for edges as struct.
 * @details It is holding two int. The first one defines the first vertex of the edge,
 * the second int the second one. The edge is directed from int a to int b.
 */
typedef struct
{
    unsigned int a; /**< startvertex of edge */
    unsigned int b; /**< endvertex of edge */
} edge;

/**
 * @brief Defines new type for an feedback arc set as struct.
 * @details An arcset is one solution of the in gnereator.c implemented algorithm.
 * It is holding one int named "size", which indicates the length of
 * the arc set. In addition stores the edge array "edges" with size of EDGE_COUNT the solution
 * set of edges which repesent the arc set. It is only possible to store maximum of EDGE_COUNT
 * edges, otherwise a solution is not good enough and therefore not valid.
 */
typedef struct
{
    int size;               /**< stores the length of the solution set. Maximum is EDGE_COUNT */
    edge edges[EDGE_COUNT]; /**< stores all edges of the solution set. */
} arcset;

/**
 * @brief struct which represents the shared memory.
 * 
 * @details so the shared memory contains the current write position since 
 * multiple generator are writing to the array we can not store the write position
 * in each prozess seperatly. Furthermore the struct holds the status of the shared memory
 * which is change by the supervisor process. 0 means ok, 1 means end.
 * Last but not least the memory stores an arcset array, which is the actual
 * circularBuffer. It has a length of BUFFER_SIZE and is written by the generator processes and
 * read by the supervisor process.
 */
struct graph_shm
{
    int wr_pos;               /**< holds current write position of the circular buffer */
    unsigned int status;      /**< represents the status of the circular buffer (0 is ok, 1 is success quit)*/
    arcset sets[BUFFER_SIZE]; /**< stores the arcset which are determine by the generators */
} * shm;

sem_t *used_sem;  /**< stores the used semaphore */
sem_t *free_sem;  /**< stores the free semaphore */
sem_t *mutex_sem; /**< stores the mutex semphore*/

/***************************
 *  SEM AND SHM FUNCTIONS  *
 ***************************/
/**
 * @brief Manages the smooth cleaning of semaphores and shm
 * 
 * @details It is repsonsible to unmap the shared memory via munmap(), closes all
 * semaphores via sem_close() and unlinks them via sem_unlink if the program is the supervisor.c programm.
 * It prints also error messages if one the functions doesnt work as expected. 
 * If no one of the globalvariables ***_set is set to true, the function will not try to close these semaphores.
 * 
 * @see err_msg()
 * 
 * @param progn is the program name. If is equal to "supervisor.c" it will unlink all semaphores.
 */
void clean_up(char *progn);

/**
 * @brief Manages the setup of the supervisor process.
 * 
 * @details Since the supervisor.c is the server and generator.c the client, supervisor.c
 * is responsible for the creation of the shared memory as well as for the semaphores.
 * For that this function opens a new shm memory with read write access, declares the 
 * size of the reservered memory and maps the graph_shm struct to the memory SHM_NAME.
 * 
 * Then the semaphores USED_SEM, FREE_SEM and MUTEX_SEM are opened and initialized with there values.
 * USED_SEM is initialized with 0 since the supervisor should only read when there is something to read.
 * FREE_SEM is initialized with BUFFER_SIZE from above since at the beginning nothin is written to the Buffer.
 * MUTEX_SEM is initialized with 1 since the first generator who writes need to decrement and after writing increment it.
 * 
 * When a semaphore is opened, the global variable ****_set are set to true.
 */
void setup_supervisor(void);

/**
 * @brief Manages the setup of the generate process.
 * 
 * @details Since generate.c is the client, it just need to link to the shared memoy and semaphores without creating it.
 * If there was no semaphore or shm created, this function exits in an error.
 * 
 * The function opens following semaphores and shm:
 * USED_SEM, FREE_SEM, MUTEX_SEM and SHM_NAME-shm.
 * 
 * When a semaphore is opened, the global variable ****_set are set to true.
 */
void setup_generator(void);

/**
 * @brief Prints all semaphores and their values
 */
void print_semaphores(void);

/**************************************
 *  ACTUAL CIRCULAR BUFFER FUNCTIONS  *
 **************************************/
/**
 * @brief writes set to circular buffer.
 * 
 * @details Decrements FREE_SEM (may be blocked), after passing FREE_SEM it decrements
 * MUTEX_SEM (may be blocked). After passing MUTEX_SEM it reads the current writing position
 * from the shm struct and writes the given set on this position.
 * Then the function calculates the new write position by incrementing the writing position and
 * calc modulo BUFFER_SIZE (length of buffer). The new value is stored as writing position on the shm.
 * 
 * After the write process it increments the MUTEX_SEM to let an other generator write and
 * increments the USED_SEM to let the supervisor process read the new arcset stored in the buffer.
 * 
 * @param set Given arcset to store in shared memory.
 * @return int which returns if the process is done or was interrupted by a signal.
 * 0 for done, -1 for interruped and not finished.
 */
int write_set(arcset set);

/**
 * @brief Reads new arcset from buffer.
 * 
 * @details A static int variable "rd_pos" holds the current reading postion, 
 * which is possible since there is only 1 supervisor process. 
 * After declaring it (if it isn't already) it decrements the USED_SEM semaphore.
 * When it passed the semaphore it reads the arcset at the
 * reading position and sets the content of the given arcset pointer to this read
 * arcset. Then it increments the semaphore to give the generators the "permission"
 * to overright the just now read arcset.
 * After this process the reading postion is incremented and calulated modulo BUFFER_SIZE
 * to gurantee that the reading position is not out of index.
 * 
 * @param set Pointer to arcset where the new set should be stored.
 * @return Returns 0 if process done and -1 if process was interrupted by signal.
 * 
 */
int read_delete_set(arcset *set);

/**
 * @brief Writes the status to shared memory
 * 
 * @details Writes a given status to the buffer to indicates that calculation process is over
 * and the supervisor was stop or found an asyclic graph.
 * 
 * @param status status to write to shared memory
 * @return 0 when done.s
 */
int set_status(int status);

/**
 * @brief Reads status from shared memory
 * 
 * @details Is excecuted bei the generator right before the writing process to
 * get the information if the process should be stopped.
 * 
 * @returns Status of shared memory. Generator exits if status=1.
 */
unsigned int get_status(void);

/**
 * @brief Prints the solution of an argset. 
 * 
 * @details Prints the solution of an arcset with all edges and the size of it.
 * 
 * @param prog Name of the program which want to print it.
 * @param set Pointer to the set which should get printed.
 */
void print_solution(const char *prog, arcset *set);

#endif //CB_H
