#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
/**
 * @file mmake.c
 * @brief A simplified implementation of the Unix 'make' utility.
 *
 * This program parses a makefile and builds the specified targets in the 
 * correct order, based on their prerequisites and timestamps. 
 * 
 * Supported features:
 * Reads a makefile (default: "mmakefile", or a file specified with -f).
 * Supports the flags:
 *   -s : Suppress command output before execution.
 *   -B : Force rebuild of all specified targets regardless of timestamps.
 * Recursively builds prerequisites before building a target.
 * Executes the commands associated with a target using fork() and execvp().
 * 
 * The program ensures that targets are only rebuilt when necessary, 
 * unless explicitly forced with the -B flag.
 *
 * @author Jonathan Davidsson
 * @date 30/10/25
 */
void build_targets(makefile *rules, const char *target, bool B_flag, bool s_flag);
void parse_flags(int argc, char *argv[], bool *s_flag, bool *B_flag, char **make_file_name);
FILE *open_makefile(const char *make_file_name);

/**
 * Function: parse_flags
 *
 * @brief Parses command-line flags using getopt.
 *
 * Updates the given flag variables and makefile name
 * based on the user’s input (-s, -B, -f).
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param s_flag Suppress output flag (-s).
 * @param B_flag Force rebuild flag (-B).
 * @param make_file_name Makefile name if given (-f).
 */
void parse_flags(int argc, char *argv[], bool *s_flag, bool *B_flag, char **make_file_name) {
    int opt;
    while ((opt = getopt(argc, argv, "sBf:")) != -1) {
        switch (opt) {
            case 's': *s_flag = true; break;
            case 'B': *B_flag = true; break;
            case 'f': *make_file_name = optarg; break;
            case '?':
                fprintf(stderr, "Okänd flagga! '-%c'...\nAnvänd något av: -s, -B, -f:\n", optopt);
                exit(EXIT_FAILURE);
        }
    }
}

/**
 * Function: open_makefile
 *
 * @brief Opens the makefile for reading.
 *
 * Opens the specified makefile or defaults to "mmakefile".
 * Exits with error if the file cannot be opened.
 *
 * @param make_file_name The name of the makefile (can be NULL).
 * @return FILE pointer to the opened makefile.
 */
FILE *open_makefile(const char *make_file_name) {
    FILE *makefile_fp;
    if(make_file_name != NULL){
		makefile_fp = fopen(make_file_name, "r");

		if(makefile_fp == NULL){
			perror("fopen");
			exit(EXIT_FAILURE);
		}

	}else{
		make_file_name = "mmakefile";
		makefile_fp = fopen(make_file_name, "r");

		if(makefile_fp == NULL){
			perror("fopen");
			exit(EXIT_FAILURE);
		}
	}

    return makefile_fp;
}

int main(int argc, char *argv[]){
	
	char *make_file_name = NULL;
	FILE *makefile_fp;

	bool s_flag = false;
	bool B_flag = false;
    
    parse_flags(argc, argv, &s_flag, &B_flag, &make_file_name);
    makefile_fp = open_makefile(make_file_name);

	makefile *rules;
	rules = parse_makefile(makefile_fp);
	if(rules == NULL){
		fprintf(stderr, "mmake: could not parse makefile\n");
		fclose(makefile_fp);
		exit(EXIT_FAILURE);
	}
	fclose(makefile_fp);
	
	int num_of_targets = argc - optind;
	const char *target = NULL;

	if(num_of_targets == 0){
		target = makefile_default_target(rules);
		build_targets(rules, target, B_flag, s_flag);
	}else{
		for(int i = optind; i < argc; i++){
			target = argv[i];
			build_targets(rules, target, B_flag, s_flag);
		}
	}
	makefile_del(rules);
	return 0; 
}

/**
 * Function: needs_rebuild
 *
 * @brief Determines if a target needs to be rebuilt.
 * 
 * @param target The target file name.
 * @param prereqs List of prerequisites.
 * @param B_flag If true, always rebuild.
 * @return true if rebuild is required, false otherwise.
 */
static bool needs_rebuild(const char *target, const char **prereqs, bool B_flag) {
    if (B_flag){
		return true;
	}

    struct stat target_stat;
    if (stat(target, &target_stat) != 0) {
        if (errno == ENOENT) {
            return true;
        } else {
            perror("stat");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; prereqs[i] != NULL; i++) {
        struct stat prereq_stat;
        if (stat(prereqs[i], &prereq_stat) == 0) {
            if (prereq_stat.st_mtime > target_stat.st_mtime) {
                return true;
            }
        } else {
            return true;
        }
    }
    return false;
}

/**
 * Function: print_command
 *
 * @brief Prints the command if not suppressed.
 *
 * @param cmd Commands to be printed
 * @param s_flag A boolean to determine if the flag -s was used.
 */
static void print_command(char **cmd, bool s_flag) {
    if (s_flag) return;
    for (int i = 0; cmd[i]; i++) {
        printf("%s", cmd[i]);
        if (cmd[i + 1]) printf(" ");
    }
    printf("\n");
}

/**
 * Function: execute_command
 *
 * @brief Executes a build command using fork/execvp.
 *
 * @param cmd The commands to be executed
 * @param rules Makefile data, used for cleanup on error.
 */
static void execute_command(char **cmd, makefile *rules) {
    if (cmd[0] == NULL){
		return;
	} 

    pid_t pid = fork();
    if (pid == 0) {
        execvp(cmd[0], cmd);
        perror("execvp");
        makefile_del(rules);
        exit(EXIT_FAILURE);
    } else {
        int status;
        if (wait(&status) == -1) {
            perror("wait");
            makefile_del(rules);
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            //fprintf(stderr, "mmake: child exited with status %d\n", WEXITSTATUS(status)); -- This line can be uncommented for specific error status. 
            makefile_del(rules);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Function: build_targets
 *
 * The function takes a list of rules and a target, and ensures that
 * the target is built in the correct order by first recursively building 
 * its prerequisites. It then determines whether the target needs to be 
 * rebuilt based on timestamps or flags, and if so, executes the commands
 * associated with the rule.
 *
 * @param rules   Parsed makefile structure containing all rules.
 * @param target  The specific target (file or rule) to build.
 * @param B_flag  If true, forces rebuilding regardless of timestamps (-B flag).
 * @param s_flag  If true, suppresses command output during build (-s flag).
 */
void build_targets(makefile *rules, const char *target, bool B_flag, bool s_flag) {
    rule *r = makefile_rule(rules, target);

    if (r == NULL) {
        struct stat st;
       if (stat(target, &st) != 0) {
            if (errno == ENOENT) {
                fprintf(stderr, "mmake: target '%s' not found\n", target);
                makefile_del(rules);
                exit(EXIT_FAILURE);
            } else {
                perror("stat");
                makefile_del(rules);
                exit(EXIT_FAILURE);
            }
        }
        return;
    }

    const char **prereqs = rule_prereq(r);
    if (prereqs == NULL) {
        fprintf(stderr, "mmake: internal error: prerequisites are NULL for target '%s'\n", target);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; prereqs[i] != NULL; i++) {
        build_targets(rules, prereqs[i], B_flag, s_flag);
    }

    if (needs_rebuild(target, prereqs, B_flag)) {
        char **cmd = rule_cmd(r);
        print_command(cmd, s_flag);
        execute_command(cmd, rules);
    }
}