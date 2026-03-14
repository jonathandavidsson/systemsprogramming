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
#include "queue.h"
/**
* File: queue.c
*
* Description:
* This file contains the implementation of a simple dynamic queue structure
* used for storing directory paths to be processed by multiple threads in the
* mdu program. The queue provides synchronized access functions for adding
* and retrieving elements in a thread-safe manner.
*
* @author Jonathan Davidsson
* @date 13/11/25
*/

int init_shared_data(shared_data_t *data, int argc, char *argv[], int optind) {
    pthread_mutex_init(&data->mtx, NULL);
    pthread_cond_init(&data->cond, NULL);
    data->done = false;
    data->total = 0;
    data->error = false;
    data->queue_capacity = argc;
    data->queue_size = 0;
    data->queue = malloc(sizeof(char*) * data->queue_capacity);
    if (!data->queue) {
        perror("malloc");
        return -1;
    }

    pthread_mutex_lock(&data->mtx);
    for (int i = optind; i < argc; i++) {
        queue_add(data, argv[i]);
    }
    pthread_mutex_unlock(&data->mtx);

    return 0;
}

/**
* Function: queue_add
*
* @brief Adds a new file path to the shared queue, expanding the queue's capacity if needed.
*
* @param data Pointer to shared_data_t structure.
* @param path The file path to be added to queue.
*/
void queue_add(shared_data_t *data, const char *path){
	if(data->queue_size == data->queue_capacity){
		int new_capacity = data->queue_capacity * 2;
		char **new_queue = realloc(data->queue, sizeof(char*) * new_capacity);
				
		if(new_queue == NULL){
			perror("realloc");
			data->error = true;
            return;
		}

		data->queue = new_queue;
		data->queue_capacity = new_capacity;
	}
    char *dup = strdup(path);
    if(!dup){
        perror("dup");
        data->error = true;
        return;
    }
    data->queue[data->queue_size++] = dup;
}

/**
* Function: cleanup_shared_data
*
* @brief Deallocates memory and destroys mutex and condition variable.
*
* @param data pointer to shared_data_t to be cleaned
* @param num_paths Number of elements in the queue to free.
*/
void cleanup_shared_data(shared_data_t *data, int num_paths) {
    pthread_mutex_destroy(&data->mtx);
    pthread_cond_destroy(&data->cond);
    for (int i = 0; i < num_paths; i++) {
        free(data->queue[i]);
    }
    free(data->queue);
}