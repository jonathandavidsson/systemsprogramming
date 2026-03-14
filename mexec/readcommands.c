#include "readcommands.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
/**
 * @file readcommands.c
 *
 * @brief Provides functions to read, parse, and free command input for mexec.c.
 *
 * Description: This file contains functions that handle input from the mexec.c program. 
 * It reads the input and stores the parsed data in a triple pointer where each line of 
 * input is split into separate command arguments. The file also contains a function to 
 * free all allocated memory once the commands have been processed.
 *
 * @author Jonathan Davidsson
 * @date 14/10/25
 */



/**
 * Function: parse_line
 *
 * @brief Splits a line into tokens and returns a NULL-terminated argv array.
 *
 * @param line Input string to parse.
 * @return Dynamically allocated argv array.
 */
static char **parse_line(char *line){
    int argc = 0;
    int argcap = 8;
    char **argv = malloc(argcap * sizeof(char *));

    if(!argv){
        perror("argv malloc error");
        exit(EXIT_FAILURE);
    }

    char *token = strtok(line," \t\n");

    while(token){
            
        if(argc >= argcap - 1){
            argcap *= 2;
            argv = realloc(argv, argcap * sizeof(char *));
            if(!argv){
                perror("Realloc argv fail.");
                free(argv);
                exit(EXIT_FAILURE);
        
            }
        }

        argv[argc++] = strdup(token);
        token = strtok(NULL, " \t\n");
    }

    argv[argc] = NULL;
    return argv;
}


char ***read_commands(FILE *input, int *num_commands){
    char buffer[1024];
    int capacity = 8;
    int count = 0;

    char ***commands = malloc(capacity * sizeof(char **));

    if(!commands){
        perror("No commands!");
        exit(EXIT_FAILURE);
    }

     while(fgets(buffer, sizeof(buffer), input) != NULL){
        char **argv = parse_line(buffer);
        
        if(count >= capacity){
            capacity *= 2;
            commands = realloc(commands, capacity * sizeof(char **));
            if(!commands){
                perror("realloc fail.");
                exit(EXIT_FAILURE);
            }
        }
        commands[count++] = argv;
    
    }
    *num_commands = count;
	
    return commands;

}

void freememory(char ***commands, int num_commands){
	for(int i = 0; i < num_commands; i++){
		for(int j = 0; commands[i][j] != NULL; j++){
			free(commands[i][j]);
		}
		free(commands[i]);
	}
	free(commands);
}