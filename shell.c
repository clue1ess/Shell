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
jobs *fg;
sigset_t mask;
static int group_no = 7	;


void init() {
	int i;
	arr = (jobs **)malloc(sizeof(jobs) * NUM_JOBS);
	for(i = 0; i < NUM_JOBS; i++)
		arr[i] = NULL;
	job_index = -1;
	//printf("%d\n", job_index);
	fg = NULL;
	return;
}

command **tokenizeString(char *str) {
	command **args;
	int i, j, l, k;
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
	for(k = 0; k < NUM_COMMANDS; k++)
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
			for(k = 0; k < NUM_COMMANDS; k++)
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
	if(job_index == -1) {
		sigprocmask(SIG_BLOCK, &mask, NULL);
		return;
	}
	if(sig == SIGINT) {
	//fprintf(stderr, "handle signal : %d %d\n", job_index, fg->pgid);

		printf("\nTerminated using CTRL-C\n");
		//pid = fg->pid[0];
		pgid = fg->pgid;
		free(arr[job_index]);
		job_index--;
		fg = NULL;
		kill(-pgid, SIGINT);
		//printf("%d",job_index);
	}
	else if(sig == SIGTSTP) {
		printf("\nSuspended using CTRL-Z\n");
		//pid = fg->pid[fg->index-1];
		pgid = fg->pgid;
		fg->status = STOPPED;
		fg = NULL;
		kill(-pgid, SIGSTOP);
		shell();
	} 
	return;
}

void changeDir(command **args) {
	char buff[128];
	char *home_dir;
	home_dir = getenv("HOME");
	//printf("%s", home_dir);
	if(!(args[0]->arguments[1])) {
		chdir(home_dir);
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

void printCmd(command **args) {
	int i, j, l;
	i = -1;
	while(args[++i]) {
		j = 0;
		printf("%s ", args[i]->cmd);
		if(!args[i]->arguments){
			continue;
		}
		while(args[i]->arguments[j+1]) {
			if(j == 0)
				j++;
			printf("%s ", args[i]->arguments[j]);	
			j++;
		}
	}
	return;
}

void noPipe(command **args, int length) {
	int pid, fd = 0, background = 0, ret, i, k, j, index, flag = 0;
	int status;
	char *str, str2[20];

	//printCommands(args);
	
	if(!(strcmp(args[length-1]->cmd, "&"))) {
		background = 1;
	}
	if(!(strcmp(args[length-1]->cmd, "bg"))) {
		free(arr[job_index]);
			job_index--;
		if(job_index == -1)
			return;
		str = args[0]->arguments[1];
		if(!str) {
			index = 1;
			//	return;
		}
		else {
			str++;
			index = atoi(str);
		}
		pid = arr[job_index-index+1]->pid[0];
		arr[job_index-index+1]->status = RUNNING;
		//printf("\n");
		printCmd(arr[job_index-index+1]->args);
		printf("  &\n");
		//printf("\n");
		kill(pid, SIGCONT);
		shell();
		return;
	}
	if(!(strcmp(args[length-1]->cmd, "fg"))) {
		free(arr[job_index]);
			job_index--;
		if(job_index == -1)
			return;
		str = args[0]->arguments[1];
		if(!str) {
			index = 1;
			//return;
		}
		else {
			str++;
			index = atoi(str);
		}
		fg = arr[job_index-index+1];
		//pid = fg->pid[fg->index-1];
		printCmd(fg->args);
		printf("\n");
		//printf("%d %d \n", fg->pgid, fg->index);
		//free(arr[job_index-index+1]);
		//printf("\n");
		background = 0;
		kill(fg->pid[fg->index-1], SIGCONT);
		tcsetpgrp(0, fg->pgid);
		waitpid(fg->pid[index-1], &status, WUNTRACED);
		tcsetpgrp(0,shellpid);
		//shell();
		return;
	}

	if(!(strcmp(args[length-1]->cmd, "jobs"))) {
		int k;
		//fprintf(stderr, "%d\n", job_index);
		free(arr[job_index]);
			job_index--;
		if(job_index == -1) {
			return;
		}
		pid = waitpid(-1, NULL, WNOHANG);
			if(pid > 0)	{
				for(k = 0; k <= job_index; k++) {
					//for(j = 0; j < index; j++) 
						if(arr[k]->pid[0] == pid) {
							arr[k]->status = COMPLETE;
						}
				}
			}
		for(k = 0; k <= job_index; k++) {

			printf("%d\t", k+1);
			switch(arr[job_index-k]->status) {
				case 1:
					printf("Stopped\t");
					break;
				case 2:
					printf("Complete\t");
					break;
				case 3:
					printf("Running\t");
					break;
				default:
					printf("status unknown\t");
					break;
			}
			printCmd(arr[job_index-k]->args);
			printf("\n");
			if(arr[k]->status == COMPLETE) {
				free(arr[k]);
				for(j = k; j < job_index; j++) {
					arr[j] = arr[j+1];
				}
				arr[job_index] = NULL;
				job_index--;
				k--;
			}
		}
		return;
	}

	pid = fork();
	if(pid == -1) {
		perror("fork failed :");
	}
	else if(pid == 0) {

		sigprocmask(SIG_UNBLOCK, &mask, NULL);
		setpgid(0, arr[job_index]->pgid);
		//arr[job_index]->pid[arr[job_index]->index++] = getpid();

		if(args[1]) {
			//printf("fjkbbk");
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
		//fprintf(stderr, "child %d ", getpid());
		ret = execvp(args[0]->cmd, args[0]->arguments);
		if(ret == -1) {
			perror("execvp failed :");
			exit(0);
		}
	}
	else {
		//printf("parent %d ", getpid());
		sigprocmask(SIG_UNBLOCK, &mask, NULL);
		setpgid(pid, arr[job_index]->pgid);
		tcsetpgrp(0, arr[job_index]->pgid);
		arr[job_index]->pid[arr[job_index]->index++] = pid;
		if(!background) {
			
			arr[job_index]->status = RUNNING;
			waitpid(pid,&status,0);
			if(WIFEXITED(status)) {
				//fprintf(stderr, "parent : %d\n", job_index);
				arr[job_index]->status = COMPLETE;
				free(arr[job_index]);
				arr[job_index] = NULL;
				fg = NULL;
				job_index--;
				tcsetpgrp(0,shellpid);
				
			}
			//wait(0);
		}
	}
	return;
}	

void withPipe(command **args, int i, int k) {
	int j = 0;
	int flag = 0, fd, background = 0, redirection = 0, ret, status;
	int  pid,  pfd1[2], pfd2[2], n;
	//fprintf(stderr, "with pipe : %d %d\n", job_index, fg->pgid);
	if(!(strcmp(args[i-1]->cmd, "&"))) {
		//fprintf(stderr, "fhgkj");
		background = 1;
	}

	n = k;
	i = k;
	i++;
	//printCmd(args);

	
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
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
			setpgid(0, arr[job_index]->pgid);
			//arr[job_index]->pid[arr[job_index]->index++] = getpid();
			
			if(i == k) { //first
				//fprintf(stderr, "%s\n", args[j]->cmd);
				//arr[job_index]->pgid = getpgrp();
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
				sigprocmask(SIG_UNBLOCK, &mask, NULL);
				setpgid(pid, arr[job_index]->pgid);
				
			arr[job_index]->pid[arr[job_index]->index++] = pid;
			if(i == k) { //first
				tcsetpgrp(0, arr[job_index]->pgid);
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
			if(!background) {
				arr[job_index]->status = RUNNING;
				waitpid(pid,&status,0);
				if(WIFSIGNALED(status)) {
					//fprintf(stderr, "signal");
					//exit(0);
					break;	
				}
				else if(i == 0 && WIFEXITED(status)) {
					//fprintf(stderr, "%d\n", job_index);
				//fprintf(stderr, "parent : %d\n", job_index);

					arr[job_index]->status = COMPLETE;
					/*for(int g = 0; g < fg->index; g++)
						printf("%d	", fg->pid[g]);
					printf("pgid : %d\n", fg->pgid);*/
					free(arr[job_index]);
					arr[job_index] = NULL;
					fg = NULL;
					job_index--;
					tcsetpgrp(0,shellpid);
				}
			}
		}
	}
}

void executeCommands(command **args) {
	int i = 0, k = 0, j, n, flag = 0, l;
	job_index++;
	arr[job_index] = (jobs *)malloc(sizeof(jobs));
	arr[job_index]->index = 0;
	arr[job_index]->pgid = ++group_no;
	arr[job_index]->args = args;
	arr[job_index]->status = RUNNING;
	fg = arr[job_index];
	sigemptyset(&mask);//Generate an empty signal set in mask
	//sigaddset(&mask, SIGCHLD);//add sigchld to blocked signal sets
	sigaddset(&mask, SIGINT);//add sigint to blocked signal sets
	sigaddset(&mask, SIGTSTP);//add sigstp to blocked signal sets
	
	if(!(strcmp(args[0]->cmd, "cd"))) {
      	sigprocmask(SIG_BLOCK, &mask, NULL); /*Blocks the signal set  in order */
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


void deleteJobs(jobs **arr) {
	int j;
	if(job_index == -1) {
		free(arr);
		return;
	}
	for(j = 0; j <= job_index; j++)
		free(arr[j]);
	free(arr);
	return;
}




	



