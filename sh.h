
#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
void *which(char *command, struct pathelement *pathlist);
char* whichRet(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void printWorkingDir();
void list ( char **args );
void printenv(char **envp);
void changeDir(char **args);
void nullify(char **args);
void printPid();
void promptCmd(char **args, char* prompt);
void killProc(char **args);

#define PROMPTMAX 32
#define MAXARGS 10
