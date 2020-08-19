/*
Features :  
    1. Multi-piping  
	2. I/O redirection  
	3. Background Commands using &  
	4. Handling Signals -> Ctrl+C and Ctrl+Z  
	5. fg, bg  
	6. Change directory -> cd  
	7. History -> history  
	8. Exit the shell -> exit    
*/
#include <stdio.h>
#include "shell.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

extern char *history[NUM_COMMANDS_BUFF];
extern jobs **arr;
int shellpid;
extern sigset_t mask;
char str[SIZE];

/*void get_input(char *str) {
	char c;
	int i = 0;
	do {
		c = getchar();
		if (c == '\t') {
			printf("fjhjk");
			str = autocomplete(str, &i);
		}
		else {
			str[i++] = c;
		}

	} while (c != '\n' && c!='\r');
	str[i] = '\0';
}*/

void about() {
	system("clear");
	printf("\n\t\t\t	MYSHELL\n\n");
	printf("\t\t--------------------------------------\n");
	printf("\t\t-              Welcome!              -\n");
	printf("\t\t--------------------------------------\n\n");
}

void shell() {
	int l;
	char buff[128];
	command **args;

	while(1) {
		getcwd(buff, 128);
		printf("gouri@prompt:%s$ ", buff);
		fgets(str, SIZE, stdin);
		//get_input(str);
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
  	action.sa_mask = mask;
	sigemptyset(&action.sa_mask);
  	action.sa_flags = SA_RESTART;

  	sigaction(SIGINT, &action, NULL);	//ctrl-c
  	sigaction(SIGTSTP, &action, NULL);  //ctrl-z*/

	shellpid = getpid(); 		//get shell pid 
	//printf("%d", shellpid);
	about();					//printing screen
	init();		//initializing job structure
	shell();
	deleteHistory(history);		//free history
	deleteJobs(arr);	//free jobs
	return 0;
}

 
