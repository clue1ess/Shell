#include <unistd.h>

#define SIZE 128
#define NUM_COMMANDS 24
#define LEN_COMMAND 64
#define NUM_PROCESS 24
#define NUM_COMMANDS_BUFF 128
#define NUM_JOBS 24
#define STOPPED 1
#define COMPLETE 2
#define RUNNING 3
#define PATH_MAX 128

typedef struct command {
	char cmd[LEN_COMMAND];
	char **arguments;
}command;

typedef struct jobs {
	command **args;
	pid_t pid[NUM_PROCESS];
	pid_t pgid;
	int index;
	int status; //1 -stopped 2 -done
}jobs;


command **tokenizeString(char *str);
void printCommands(command **args);
void executeCommands(command **args);
void getHistory();
void shell();
void handleSignal(int sig);
void init();
void deleteHistory(char **args);
void deleteJobs(jobs **arr);	
