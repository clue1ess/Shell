#include <stdio.h>
#include "shell.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

extern char *history[NUM_COMMANDS_BUFF];
extern jobs **arr;
int shellpid;

void about() {
	system("clear");
	printf("\n\t\t	SHELL\n\n");
	printf("------------------------------------------------\n\n");
	printf("Features : \n1. Multi-piping\n");
	printf("2. I/O redirection\n");
	printf("3. Background Commands using &\n");
	printf("4. Handling Signals -> Ctrl+C and Ctrl+Z\n");
	printf("5. fg, bg\n");
	printf("6. Change directory -> cd\n");
	printf("7. History -> history\n");
	//printf("8. Environment Variables -> export and unset\n");
	printf("8. Exit the shell -> exit\n\n\n");
}

void shell() {
	int l;
	char str[SIZE], buff[128];
	command **args;

	struct sigaction action;
  	action.sa_handler = handleSignal;
  	action.sa_flags = SA_RESTART;

	sigaction(SIGINT, &action, NULL);	//ctrl-c
  	sigaction(SIGTSTP, &action, NULL);  //ctrl-z

	init();		//initializing job structure

	while(1) {
		getcwd(buff, 128);
		printf("gouri@prompt:%s$ ", buff);
		fgets(str, SIZE, stdin);
		l = strlen(str);
		str[l-1] = '\0';
		if(!strcmp(str, "\0"))
			continue; 
		else if(!strcmp(str, "history")) {
			getHistory();
			continue;
		}
		else if(!strcmp(str, "exit")) {
			printf("Exiting...\n");
			sleep(1);
			exit(0);
		}
		else if(!strcmp(str, "help")) {
			printf("Type exit to terminate the shell\n");
			continue;
		} 
		args = tokenizeString(str);
		//printCommands(args);
		executeCommands(args);
		//break;
	}	
	return;
}


int main() {
	
	struct sigaction action;
  	action.sa_handler = handleSignal;
  	action.sa_flags = SA_RESTART;

  	sigaction(SIGINT, &action, NULL);	//ctrl-c
  	sigaction(SIGTSTP, &action, NULL);  //ctrl-z

	shellpid = getpid(); 		//get shell pid 
	about();					//printing screen
	shell();
	deleteHistory(history);		//free history
	deleteJobs(arr);			//free jobs
	return 0;
}

 
