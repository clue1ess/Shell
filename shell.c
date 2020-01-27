#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shell.h"
#include <signal.h>


char *history[NUM_COMMANDS_BUFF];
int hist_index = 0;
jobs **arr;
extern int shellpid;
int job_index = -1;


void init() {
	arr = (jobs **)malloc(sizeof(jobs) * NUM_JOBS);
	return;
}

void deleteJobs(jobs **arr) {//incomplete
}

command **tokenizeString(char *str) {
	command **args;
	int i, j, l;
	char *token;
	if(hist_index == 0 || strcmp(str, history[hist_index-1])) {
		history[hist_index++] = (char *)malloc(strlen(str) + 1);
		strcpy(history[hist_index-1], str);
	}
	args = (command **)malloc(sizeof(command *) * NUM_COMMANDS);
	for(i = 0; i < NUM_COMMANDS; i++) 
		args[i] = NULL;
	i = 0;
	token = strtok(str, " ");
	args[i] = (command *)malloc(sizeof(command));
	strcpy(args[i]->cmd, token);
	args[i]->arguments = NULL;
	j = 0;
	args[i]->arguments = (char **)malloc(sizeof(char *) * NUM_COMMANDS);
	for(int k = 0; k < NUM_COMMANDS; k++)
		args[i]->arguments[k] = NULL;
	args[i]->arguments[j] = (char *)malloc(sizeof(char) * (strlen(args[i]->cmd) + 1));
	strcpy(args[i]->arguments[j], args[i]->cmd);
	j++;
	i++;
	while(token) {
		token = strtok(NULL, " ");
		if(!token) {
			break;
		}
		l = strlen(token);
		if(token[0] == '\'' || token[0] == '\"') {
			token++;
			token[l-2] = '\0';
		}
 		if(!strcmp(token, "|") || !strcmp(token, ">") || !strcmp(token, "<") || !strcmp(token, "&")) {
			args[i] = (command *)malloc(sizeof(command));
			strcpy(args[i]->cmd, token);
			args[i]->arguments = NULL;
			j = 0;
			i++;
		}
		else if(j == 0) {
			args[i] = (command *)malloc(sizeof(command));
			strcpy(args[i]->cmd, token);
			args[i]->arguments = (char **)malloc(sizeof(char *) * NUM_COMMANDS);
			for(int k = 0; k < NUM_COMMANDS; k++)
				args[i]->arguments[k] = NULL;
			args[i]->arguments[j] = (char *)malloc(sizeof(char) * (strlen(args[i]->cmd) + 1));
			strcpy(args[i]->arguments[j], args[i]->cmd);
			j++;
			i++;
		}
		else if(j != 0) {
			args[i-1]->arguments[j] = (char *)malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(args[i-1]->arguments[j], token);
			j++;
		}
	}
	
	return args;
}


void printCommands(command **args) {
	int i = -1, j;
	while(args[++i]) {
		j = 0;
		printf("%s	", args[i]->cmd);
		if(!args[i]->arguments){
			printf("\n");
			continue;
		}
		while(args[i]->arguments[j]) {
			printf("%s	", args[i]->arguments[j]);	
			j++;
		}
		printf("\n");
	}
	return;
}

void deleteHistory(char **args) {
	int l;
	for(l = 0; l < hist_index; l++) {
		free(history[l]);
	}
	return;
}

void getHistory() {
	int l;
	for(l = 0; l < hist_index; l++) {
		printf("%s", history[l]);
		printf("\n");
	}
	return;
}

pid_t getPgid() {
	return arr[job_index]->pgid;
}

void handleSignal(int sig) {
	pid_t pgid, pid;
	if(sig == SIGINT) {
		printf("\nTerminated using CTRL-C\n");
		pid = getPid(2);
		kill(pid, SIGTERM);
	}
	else if(sig == SIGTSTP) {
		printf("\nSuspended using CTRL-Z\n");
		pid = getPid(1);
		kill(pid, SIGSTOP);
		//signal(SIGSTOP, SIG_DFL);
		shell();
	} 
	return;
}

void changeDir(command **args) {
	char buff[128];
	if(!(args[0]->arguments[1])) {
		chdir("/home/hp");
		return;
	}
	if(!strcmp(args[0]->arguments[1], "/")) {
		chdir(args[0]->arguments[1]);
		return;
	}
	getcwd(buff, 128);
	strcat(buff, "/");
	strcat(buff, args[0]->arguments[1]);
	chdir(buff);
	return;
}

pid_t getPid(int mode) {	//0 - fg, 1 - bg
	arr[job_index]->mode = mode;
	return arr[job_index]->pid[arr[job_index]->index];
}


void noPipe(command **args, int length) {
	int pid, fd = 0, background = 0, ret, i, k;
	int status;
	char *str;
	arr[job_index] = (jobs *)malloc(sizeof(jobs));
	arr[job_index]->index = 0;
	arr[job_index]->args = args;
	arr[job_index]->mode = 0;

	//printCommands(args);
	if(!(strcmp(args[length-1]->cmd, "&"))) {
		background = 1;
	}
	if(!(strcmp(args[length-1]->cmd, "bg"))) {
		str = args[0]->arguments[1];
		if(!str) {
			fprintf(stderr, "provide pid number\n");
			return;
		}
		else 
			str++;
		pid = atoi(str);
		kill(pid, SIGCONT);
		shell();
		return;
	}
	if(!(strcmp(args[length-1]->cmd, "fg"))) {
		str = args[0]->arguments[1];
		if(!str) {
			fprintf(stderr, "provide pid number\n");
			return;
		}
		else 
			str++;
		pid = atoi(str);
		kill(pid, SIGCONT);
		signal(SIGTTOU, SIG_IGN);
		tcsetpgrp(0, getpgid(pid));
		signal(SIGTTOU, SIG_DFL);
		waitpid(getpgid(pid), NULL, WUNTRACED);
		signal(SIGTTOU, SIG_IGN);
		tcsetpgrp(0, getpgid(shellpid));
		signal(SIGTTOU, SIG_DFL);
		return;
	}

	if(!(strcmp(args[length-1]->cmd, "jobs"))) {
		if(job_index == -1)
		for(k = 0; k < job_index; k++);
	}

	pid = fork();
	if(pid == -1) {
		perror("fork failed :");
	}
	else if(pid == 0) {
		setpgrp();
		//arr[job_index]->pid  = (pid_t *)malloc(sizeof(pid_t) * NUM_PROCESS);
		arr[job_index]->pid[arr[job_index]->index] = getpid();
		//fprintf(stderr, "child : %d\n", arr[job_index]->pid[arr[job_index]->index]);
		arr[job_index]->pgid = getpgrp();

		signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
		if(args[1]) {
			if(!strcmp(args[1]->cmd, ">")) {
				//output redirection
				close(1);
				fd = open(args[2]->cmd,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR 
				| S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			}
			else if(!strcmp(args[1]->cmd, "<")) {
				//input redirection
				close(0);
				fd = open(args[2]->cmd, O_RDONLY, S_IRUSR | S_IWUSR 
				| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
				
				
			}
		}
		ret = execvp(args[0]->cmd, args[0]->arguments);
		if(ret == -1) {
			perror("execvp failed :");
			exit(0);
		}
	}
	else {
		setpgid(pid, pid);
		arr[job_index]->pid[arr[job_index]->index] = pid;
		//fprintf(stderr, "child : %d %d\n", arr[job_index]->pid[arr[job_index]->index], getpid());
		if(!background) {
			arr[job_index]->mode = 1;
			wait(0);
		}
		else {
			waitpid(pid,&status,0);
			//fprintf(stderr, "%d\n",pid);	
			//arr[job_index]->mode = 2;
		}
	}
	return;
}	

void withPipe(command **args, int i, int k) {
	int j = 0;
	int flag = 0, fd, background = 0, redirection = 0, ret;
	int  pid,  pfd1[2], pfd2[2], n;
	
	arr[job_index] = (jobs *)malloc(sizeof(jobs));
	arr[job_index]->index = -1;
	arr[job_index]->args = args;
	arr[job_index]->mode = 0;
	n = k;
	i = k;
	i++;

	if(!(strcmp(args[i-1]->cmd, "&"))) {
		background = 1;
		arr[job_index]->mode = 1;
	}

	j = -2;
	while(i--) {
			j = j + 2;
		if(j == 2) {
			if(!strcmp(args[j-1]->cmd, "<")) {
				j = j + 2;
			}
		}
		if(i == k) {
			pipe(pfd1);
		}
		else if(!flag) {
			pipe(pfd2);
			flag = 1;
		}
		else {
			pipe(pfd1);
			flag = 0;
		}
		pid = fork();
		if(pid == -1) {
			perror("fork failed :");
		}
		else if(pid == 0) {
			setpgrp();
		//arr[job_index]->pid  = (pid_t *)malloc(sizeof(pid_t) * NUM_PROCESS);
		arr[job_index]->pid[++arr[job_index]->index] = getpid();
		//fprintf(stderr, "child : %d\n", arr[job_index]->pid[arr[job_index]->index]);
		arr[job_index]->pgid = getpgrp();

		signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
			if(i == k) { //first
				//fprintf(stderr, "%s\n", args[j]->cmd);
				if(args[j+1]) {
					if(!strcmp(args[j+1]->cmd, "<")) {
						close(0);
						fd = open(args[j+2]->cmd,   O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP 
						| S_IWGRP | S_IROTH | S_IWOTH);
						//fprintf(stderr, "%s\n", args[j+2]->cmd);
					}
				} 
				//fprintf(stderr, "%s\n", args[j]->cmd);
				close(1);
				dup(pfd1[1]);
				close(pfd1[0]);
			}
			else if(i == 0) {	//last
					if(flag) { //odd no of pipes -->1
						//fprintf(stderr, "%s\n", args[j]->cmd);
						if(args[j+1]) {
							if(!strcmp(args[j+1]->cmd, ">")) {
								close(1);
								fd = open(args[j+2]->cmd,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR |
								 S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
								redirection = 1;
								
							}
						} 
					//	fprintf(stderr, "%s\n", args[j]->cmd);
						close(0);
						dup(pfd1[0]);
						close(pfd2[1]);
						close(pfd2[0]);
						close(pfd1[1]);

					}
					else { //even no of pipes --> 2
						if(args[j+1]) {
							if(!strcmp(args[j+1]->cmd, ">")) {
								close(1);
								fd = open(args[j+2]->cmd,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR |
								 S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
								redirection = 1;
							}
						} 
						close(0);
						dup(pfd2[0]);
						close(pfd2[1]);
						close(pfd1[0]);
						close(pfd1[1]);
					}
			}
			else{	//middle

					if(flag) { 		//1 ---> 2
						close(0);
						dup(pfd1[0]);
						close(1);
						dup(pfd2[1]);

						close(pfd2[0]);

					}
					else {			//2 ---> 1
						close(0);
						dup(pfd2[0]);
						close(1);
						dup(pfd1[1]);
					}

			}

			ret = execvp(args[j]->cmd, args[j]->arguments);
			if(ret == -1) {
				perror("execvp failed :");
				exit(0);
			}
			exit(0);
			//if(redirection)
			//	return;
			
		}
		else {
			setpgid(pid, pid);
			arr[job_index]->pid[++arr[job_index]->index] = pid;
			if(i == k) { //first
				close(pfd1[1]);
			}
			else if(i == 0) {	//last
					if(flag) { //odd no of pipes -->1
						close(pfd1[0]);
						close(pfd2[1]);
						close(pfd2[0]);
						close(pfd1[1]);
					}
					else { //even no of pipes --> 2
						close(pfd2[0]);
						close(pfd2[1]);
						close(pfd1[0]);
						close(pfd1[1]);
					}
			}
			else{	//middle

					if(flag) { 		//1 ---> 2
						close(pfd1[0]);
						close(pfd2[1]);

					}
					else {			//2 ---> 1
						close(pfd2[0]);
						close(pfd1[1]);
					}

			}
		//	fg_pid = getpid();
			if(!background)
				wait(0);
		}
	}
}

void executeCommands(command **args) {
	int i = 0, k = 0, j, n, flag = 0, l;
	job_index++;
	if(!(strcmp(args[0]->cmd, "cd"))) {
		changeDir(args);
		return;
	}
	
	
	i = 0;
	while(args[i]) {
		if(!(strcmp(args[i]->cmd, ">"))) { //output
			i = i + 2;
			while(args[i]) {
				j = 0;
				free(args[i]->cmd);
				if(!args[i]->arguments){
					args[i] = NULL;
					i++;
					continue;
				}
				while(args[i]->arguments[j]) {
					free(args[i]->arguments[j]);
					j++;
				}
				free(args[i]->arguments);
				args[i] = NULL;
				i++;
			}
			break;
		}
		else if(!(strcmp(args[i]->cmd, "<"))) {  //input
			//printf("kdnkjf");
			j = i - 1;
			i = 0;
			flag = 0;
			while(args[i]->cmd) {
				flag++;
				i++;
			}
			i = 0;
			//printf("%d\n", flag);
			while(i != j) {
				l = 0;
				//printf("%s\n", args[i]->cmd);
				free(args[i]->cmd);
				if(args[i]->arguments) {
					while(args[i]->arguments[l]) {
						free(args[i]->arguments[l]);
						l++;
					}
					free(args[i]->arguments);
					//args[i] = NULL;
				}
				i++;
			}
			i = 0;
			while(i < flag) {
				args[i++] = args[j++];
			}
			while(i < NUM_COMMANDS)
				args[i++] = NULL;
			break;
		}
		i++;
	}
	i = 0;
	k = 0;
	while(args[i]) {
		if(!(strcmp(args[i]->cmd, "|")))
			k++;
		i++;
	}
	flag = i;
	if(!k) {
		noPipe(args, i);
		return;	
	}
	//printCommands(args);
	withPipe(args, i, k);
	
}




	



