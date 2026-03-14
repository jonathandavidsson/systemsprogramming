#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

/* Pipeline function, which creates child processes */

void pipeline(char ***commands, int num_commands){

	int pipes[num_commands - 1][2];
	for(int i = 0; i < num_commands - 1; i++){
		if(pipe(pipes[i]) == -1){
			perror("Pipe");
			exit(EXIT_FAILURE);
		}
	}

	for(int i = 0; i < num_commands; i++){
		int pid = fork();
		if(pid == -1){
			perror("Fork");
			exit(EXIT_FAILURE);
		}
		//Kolla om vi är i ett barn.
		if(pid == 0){
			//Här måste vi kolla om vi är i första kommandot eller inte alltså om i > 0, där i är vilket kommando de är.
			if(pid > 0){
				//Då om de inte är första kommandot läs i så fall från föregående pipe.
				dup2(pipes[i - 1][0], STDIN_FILENO);
			}

			//Kolla om vi är i sista kommdot, om vi inte är det så skriv till nästa pipe. 
			if(i < num_commands - 1){
				dup2(pipes[i - 1][0], STDOUT_FILENO);
			}
			//Stäng sedan alla pipes.
			for(int j = 0; j < num_commands - 1; j++){
				close(pipes[j][0]);
				close(pipes[j][1]);
			}

			execp(commands[i][0], commands[i]);

			//Ev error check efter för att se att det funkat.
		}

		//Få föräldern att stänga sina pipes
		for(int i = 0; i < num_commands - 1; i++){
				close(pipes[i][0]);
				close(pipes[i][1]);
			}
		//Använd wait för att vänta på alla barn.
		for(int i = 0; i < num_commands - 1; i++){
			wait();
		}
	}

}
