#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

typedef struct {
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    char **queue;  
    int queue_size;
    int queue_capacity;
    bool done;
	bool error;
    unsigned long total;
} shared_data_t;

/**
* Function: queue_add
*
* @brief Adds a new file path to the shared queue, expanding the queue's capacity if needed.
*
* @param data Pointer to shared_data_t structure.
* @param path The file path to be added to queue.
*/
void queue_add(shared_data_t *data, const char *path);
/**
* Function: cleanup_shared_data
*
* @brief Deallocates memory and destroys mutex and condition variable.
*
* @param data pointer to shared_data_t to be cleaned
* @param num_paths Number of elements in the queue to free.
*/
void cleanup_shared_data(shared_data_t *data, int num_paths);
/**
* Function: init_shared_data
*
* @brief Initializes a shared_data_t struct aswell as allocates memory for it.
*
* Allocates memory for the queue, initializes mutex and condition variable, and
* adds all file arguments to the queue.
*
* @param data Pointer to the shared_data_t to be initialized
* @param argc The number of arguments.
* @param argv Array of command arguments.
* @param optind Index in argv where the file arguments start.
*/
int init_shared_data(shared_data_t *data, int argc, char *argv[], int optind);

#endif