#ifndef READCOMMANDS_H
#define READCOMMANDS_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>

/**
 * Function: read_commands
 *
 * This function reads an input file until EOF. It will save these commands in a triple  pointer
 * after this is done it will also parse the commands line by line.
 *
 * @param input The file containing the commands to be read and saved.
 * @param num_commands The number of commands.  
 * @return This function returns a tripple pointer containing the 
 */

char ***read_commands(FILE *input, int *num_commands);

/**
 * Function: freememory
 *
 * Deallocates all memory used for storing the parsed commands, including
 * individual argument strings, the argument arrays, and the top-level array.
 *
 * @param commands The tripple pointer containing the read commands.
 * @param num_commands The number of  read commands from the file.
 * @return This function does  not  return anything.
 */
void freememory(char ***commands, int num_commands);

#endif