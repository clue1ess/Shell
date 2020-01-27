#include <unistd.h>

#define SIZE 128
#define NUM_COMMANDS 24
#define LEN_COMMAND 64
#define NUM_PROCESS 24
#define NUM_COMMANDS_BUFF 128
#define NUM_JOBS 24

typedef struct command {
	char cmd[LEN_COMMAND];
	char **arguments;
}command;

typedef struct jobs {
	int mode;	//0 - fg and 1 - bg
	command **args;
	pid_t pid[NUM_PROCESS];
	pid_t pgid;
	int index;
}jobs;


command **tokenizeString(char *str);
void printCommands(command **args);
void executeCommands(command **args);
void getHistory();
void shell();
void handleSignal(int sig);
void init();
pid_t getPid(int mode);
void deleteHistory(char **args);
void deleteJobs(jobs **arr);	