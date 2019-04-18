#include "get_path.h"

typedef struct node
{ 
    pthread_t thread;
    struct stat *sb1;
    char *data; 
    struct node *next; 
}node;
typedef struct nodeu
{
    int isLogged;
    char *data;
    struct nodeu *next;
}nodeu;

int pid;
int sh( int argc, char **argv, char **envp);
void *which(char *command, struct pathelement *pathlist);
char* whichRet(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void printWorkingDir();
void list ( char **args );
void printenv(char **envp, char **args,int forcePrintAll);
void setEnviornment(char **envp, char**args, struct pathelement *pathlist);
void changeDir(char **args);
void nullify(char **args);
void printPid();
void promptCmd(char **args, char* prompt);
void killProc(char **args);
void *watchmailthread(char **args);
void watchmail(char **args);
void addnode(pthread_t thread, char *data);
void addnodeu(int thread, char *data);
void watchuser(char **args);
void *watchuserthread(char **args);
void builtIns(char**args, struct pathelement* pathlist,char*prompt, char **envp);
int isBuiltin(char* command);
void forkit(char**o_args, char **envp,struct pathelement *pathlist,char*copy, int numArgs, char* prompt);

#define PROMPTMAX 32
#define MAXARGS 10