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
* File: mdu.c
*
* Description: 
* This program is a simplified, multithreaded version of the Unix 'du' (disk usage) utility. 
* It recursively traverses directories and calculates the total disk space used by files 
* and subdirectories. The program can be executed in single-threaded mode or multithreaded 
* mode using the '-j' flag followed by the number of threads.
*
* Each thread retrieves directory paths from a shared queue, processes their contents, 
* and updates a global total size counter. Synchronization between threads is handled 
* using mutexes and condition variables to ensure thread safety.
*
* @author Jonathan Davidsson
* @date 13/11/25
*/

int read_files(const char *path, unsigned long *total, shared_data_t *data, bool is_threaded);
void *worker_thread(void *arg);
void wait_for_completion(shared_data_t *data, int num_of_threads, pthread_t *threads);
int start_threads(pthread_t *threads, shared_data_t *data, int num_of_threads);
int parse_args(int argc, char *argv[], int *num_of_threads, bool *j_flag, int *optind_ptr);

/**
* Function: read_files
*
* @brief Reads a given file/directory and its size, and adds it to a total value. 
*
* Reads a given file/directory and its size, if it's a directory it will read all files in the directory and
* potential sub directorys. The function will call recursivly until no files or directorys remain. If an error
* occurs the program will update a error variable which will be returned when finished. And rest of the files will
* be traversed.
*
* @param path The current file path.
* @param total The current total value of the size.
* @param data Pointer to the shared_data_t structure.
* @param is_threaded Boolean, true if the program is using threads, false if it doesn't.
* @return This function returns an integer, either 0 or -1 depending on if a error has occured or not.
 */
int read_files(const char *path, unsigned long *total, shared_data_t *data, bool is_threaded){

	struct dirent *entry;
	struct stat st;
	DIR *dir;
	int has_error = 0;

	if(lstat(path, &st) == 0){
		*total += (unsigned long)st.st_blocks;

		if(S_ISDIR(st.st_mode)){
			dir = opendir(path);
			if(!dir){
				fprintf(stderr, "du: cannot read directory '%s': Permission denied\n", path);
                if(data){
                    pthread_mutex_lock(&data->mtx);
                    data->error = true;
                    pthread_mutex_unlock(&data->mtx);
                }
				return -1;
			}
            errno = 0;
			while ((entry = readdir(dir)) != NULL) {
				if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
					continue;
				}

				char fullpath[PATH_MAX];
				if(snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name) >= (int)sizeof(fullpath)){
                    fprintf(stderr, "path too long: %s/%s\n", path, entry->d_name);
                    data->error = true;
                    continue;
                }

				if(lstat(fullpath, &st) == 0){
					if(S_ISDIR(st.st_mode)){
						if(is_threaded){
							pthread_mutex_lock(&data->mtx);
                            queue_add(data, fullpath);
                            pthread_cond_signal(&data->cond);
                            pthread_mutex_unlock(&data->mtx);
						}else{
							if(read_files(fullpath, total, data, is_threaded) != 0){
								has_error = -1;
							}
						}
					}else{
						*total += (unsigned long)st.st_blocks; 
					}
				}else{
                    perror("lstat");
                    if(data){
                        pthread_mutex_lock(&data->mtx);
                        data->error = true;
                        pthread_mutex_unlock(&data->mtx);
                    }
                    return -1; 
                }
    		}
            if(errno != 0){
                has_error = -1;
                if(data){
                    pthread_mutex_lock(&data->mtx);
                    data->error = true;
                    pthread_mutex_unlock(&data->mtx);
                }
            }
			closedir(dir);
		}
	} else{
		perror("lstat");
		return -1;
	}
	return has_error;
}
/**
* Function: worker_thread
*
* @brief Worker function executed by each thread.
*
* Each thread continuously retrieves directory paths from the shared queue,
* processes their contents by calling read_files(), and accumulates the total
* disk usage locally. When no more work remains and the 'done' flag is set, 
* the thread exits. The local total is then added to the shared total.
*
* @param arg Pointer to the shared_data_t structure. 
* @return NULL (no return value).
*/
void *worker_thread(void *arg){

	unsigned long local_total = 0;
	shared_data_t *data = arg;

	while(1){
		pthread_mutex_lock(&data->mtx);
    	while (data->queue_size == 0 && !data->done) {
        	pthread_cond_wait(&data->cond, &data->mtx);
    	}

    	if (data->done && data->queue_size == 0) {
        	pthread_mutex_unlock(&data->mtx);
        	break; 
    	}

    	data->queue_size--;
    	char path[PATH_MAX];
    	strcpy(path, data->queue[data->queue_size]);
        if (data->queue_size == 0) {
            pthread_cond_signal(&data->cond);
        }
    	pthread_mutex_unlock(&data->mtx);

    	if(read_files(path, &local_total, data, true) != 0){
			pthread_mutex_lock(&data->mtx);
			data->error = true;
			pthread_mutex_unlock(&data->mtx);
		}
	}

	pthread_mutex_lock(&data->mtx);
    data->total += local_total;
    pthread_mutex_unlock(&data->mtx);

	return NULL;
}
/**
* Function: parse_args
*
* @brief Parses command arguments and checks for flags.
*
* @param argc Number of argunments.
* @param argv Command arguments.
* @param num_of_threads Number of threads specified by the user.
* @param j_flag Boolean, true if -j flag was used.
* @param optind Index in argv where the file arguments start
*/
int parse_args(int argc, char *argv[], int *num_of_threads, bool *j_flag, int *optind_ptr) {
    int opt;
    *j_flag = false;
    *num_of_threads = 0;

    while ((opt = getopt(argc, argv, "j:")) != -1) {
        switch (opt) {
            case 'j':
                *j_flag = true;
                *num_of_threads = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Non recognized flag! Use '-j'!\n");
                return -1;
        }
    }
    *optind_ptr = optind;

    return 0;
}
/**
* Function: start_threads
*
* @brief Creates and starts a given number of threads that runs the function worker_thread
*
* @param threads Array of pthread_t where thread ID is stored.
* @param data Pointer to shared_data_t to be split between threads.
* @param num_of_threads Number of threads to be created.
*
*/
int start_threads(pthread_t *threads, shared_data_t *data, int num_of_threads) {
    for (int i = 0; i < num_of_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, data) != 0) {
            perror("pthread_create");
            return -1;
        }
    }
    return 0;
}
/**
* Function: wait_for_completion
*
* @brief Waits until the queue is empty and all threads have completed.
*
* Locks the mutex, waits until all directorys/files have been processed, 
* marks done = true and sends a signal to all threads before join.
*
* @param data Pointer to shared_data_t.
* @param num_of_threads Number of threads to be joined.
* @param threads Array of pthread_t containing thread IDs.
*/
void wait_for_completion(shared_data_t *data, int num_of_threads, pthread_t *threads) {
    pthread_mutex_lock(&data->mtx);
    while (data->queue_size > 0) {
        pthread_cond_wait(&data->cond, &data->mtx);
    }
    data->done = true;
    pthread_cond_broadcast(&data->cond);
    pthread_mutex_unlock(&data->mtx);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: ./mdu {FILE}\n");
        exit(EXIT_FAILURE);
    }

    int num_of_threads, optind_val;
    bool j_flag, error = false;

    if(parse_args(argc, argv, &num_of_threads, &j_flag, &optind_val) != 0){
        fprintf(stderr, "Failed to read and parse command arguments\n");
        return EXIT_FAILURE;
    }

    if (j_flag) {
        shared_data_t data;
        pthread_t threads[num_of_threads];

        if(init_shared_data(&data, argc, argv, optind_val) != 0){
            fprintf(stderr, "Failed to initialize shared data\n");
            return EXIT_FAILURE;
        }
        if(start_threads(threads, &data, num_of_threads) != 0){
            fprintf(stderr, "Failed to start threads\n");
            return EXIT_FAILURE;
        }
        wait_for_completion(&data, num_of_threads, threads);

        printf("%lu\t%s\n", data.total, argv[optind_val]);
        error = data.error;

        cleanup_shared_data(&data, argc - optind_val);

    } else {
        unsigned long total;
        for (int i = optind_val; i < argc; i++) {
            total = 0;
            if (read_files(argv[i], &total, NULL, false) != 0) {
                error = true;
            }
            printf("%lu\t%s\n", total, argv[i]);
        }
    }

    return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
