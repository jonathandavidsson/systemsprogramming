#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "readcommands.h"

/**
 * @file mexec.c
 *
 * @brief Executes a pipeline of commands read from a file or standard input.
 *
 * This file executes a pipeline. It takes a file as input and reads the file for
 * command arguments which are executed with the help of child processes. If no file is provided
 * the program reads from standard input. The command arguments are parsed and stored in an array,
 * which is then used by the child processes that execute the commands using exec. After execution
 * the pipes are closed and all allocated memory is freed.
 *
 * @author Jonathan Davidsson
 * @date 14/10/25
 *
*/

/**
 * Function: close_all_pipes
 *
 * @brief Closes all pipe file descriptors.
 *
 * Closes both the read and write ends of all pipes used in the pipeline.
 * This must be called in both parent and child processes to prevent
 * file descriptor leaks.
 *
 * @param pipes The 2D array containing pipe file descriptors.
 * @param num_commands The number of commands in the pipeline.
 * @return This function does not return anything!
 */
static void close_all_pipes(int pipes[][2], int num_commands){
    for(int j = 0; j < num_commands - 1; j++){
        close(pipes[j][0]);
        close(pipes[j][1]);
    }
}

/**
 * Function: execute_command
 *
 * @brief Executes a single command in a child process.
 *
 * Sets up the necessary I/O redirections, closes all pipe file descriptors,
 * and executes the command using execvp(). If execvp() fails, prints an
 * error and exits.
 *
 * @param commands The triple pointer containing all commands.
 * @param pipes The 2D array containing pipe file descriptors.
 * @param command_index The index of the command to execute.
 * @param num_commands The total number of commands in the pipeline.
 * @return This function does not return anything!
 */
static void execute_command(char ***commands, int pipes[][2], int command_index, int num_commands){
	if(command_index > 0){
        if(dup2(pipes[command_index - 1][0], STDIN_FILENO) == -1){
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    if(command_index < (num_commands - 1)){
        if(dup2(pipes[command_index][1], STDOUT_FILENO) == -1){
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }
    close_all_pipes(pipes, num_commands);
    execvp(commands[command_index][0], commands[command_index]);
	fprintf(stderr, "%s: No such file or directory\n", commands[command_index][0]);
    exit(EXIT_FAILURE);
}

/**
 * Function: create_child_processes
 *
 * @brief Creates child processes for each command in the pipeline.
 *
 * Forks a child process for each command. In each child process, the command
 * is executed with appropriate I/O redirection through pipes. The parent
 * process stores the child PIDs for later use.
 *
 * @param commands The triple pointer containing the commands to be executed.
 * @param pipes The 2D array containing pipe file descriptors.
 * @param pids Array to store the process IDs of child processes.
 * @param num_commands The number of commands in the pipeline.
 * @return This function does not return anything!
 */
static void create_child_processes(char ***commands, int pipes[][2], pid_t pids[], int num_commands){
    for(int i = 0; i < num_commands; i++){
        pids[i] = fork();
        if(pids[i] == -1){
            perror("Fork");
            exit(EXIT_FAILURE);
        }
        if(pids[i] == 0){
            execute_command(commands, pipes, i, num_commands);
        }
    }
}

/**
 * Function: wait_for_children
 *
 * @brief Waits for all child processes to complete.
 *
 * Waits for each child process in the pipeline to finish execution.
 * Checks the exit status of each child and reports errors if any
 * process fails.
 *
 * @param pids Array containing the process IDs of child processes.
 * @param num_commands The number of child processes to wait for.
 * @return This function does not return anything!
 */
static void wait_for_children(pid_t pids[], int num_commands){
	int exit_failure = 0;
    for(int i = 0; i < num_commands; i++){
        int status;
        if(waitpid(pids[i], &status, 0) == -1){
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if(WIFEXITED(status) && WEXITSTATUS(status) != 0){
            exit_failure = 1;
        }
    }
	if(exit_failure == 1){
		exit(EXIT_FAILURE);
	}
}

/**
 * Function: pipeline
 *
 * @brief Executes a sequence of commands connected by pipes.
 *
 * Creates a pipeline of child processes using fork() and pipe(). Each command
 * is executed with execvp() in its own process. Standard input/output is
 * redirected through the appropriate pipe ends to connect the commands.
 *
 * @param commands The tripple pointer containing the commands to be executed.
 * @param num_commands The number of commands.
 * @return This function does not return anything!
 */
static void pipeline(char ***commands, int num_commands){
    pid_t pids[num_commands];
    int pipes[num_commands - 1][2];
    
    for(int i = 0; i < num_commands - 1; i++){
        if(pipe(pipes[i]) == -1){
            perror("Pipe");
            exit(EXIT_FAILURE);
        }
    }
    
    create_child_processes(commands, pipes, pids, num_commands);
    close_all_pipes(pipes, num_commands);
    wait_for_children(pids, num_commands);
}

/**
 * Function: open_input_file
 *
 * @brief Opens the input file or returns stdin.
 *
 * Opens the file specified in the command line arguments for reading.
 * If no file is specified, returns stdin. Exits with an error if the
 * file cannot be opened.
 *
 * @param argc The number of command line arguments.
 * @param argv The array of command line argument strings.
 * @return A FILE pointer to the opened file or stdin.
 */
static FILE* open_input_file(int argc, char *argv[]){
    FILE *input;
    
    if(argc > 2){
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if(argc == 2){
        input = fopen(argv[1], "r");
        if(input == NULL){
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    } else{
        input = stdin;
    }
    
    return input;
}

int main(int argc, char *argv[]){
    FILE *input = open_input_file(argc, argv);
    
    int num_commands = 0;
    char ***commands = read_commands(input, &num_commands);
    
    if(input != stdin){
        fclose(input);
    }
    
    pipeline(commands, num_commands);
    freememory(commands, num_commands);
    
    return EXIT_SUCCESS;
}