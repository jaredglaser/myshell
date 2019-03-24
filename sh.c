#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "sh.h"
#include <errno.h>


int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char input[64];

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/
     
  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */

    //generate directory
    char* workingdir;
    workingdir = getcwd(NULL, 0);
    printf("[%s]",workingdir);
    printf(">>");
    /* get command line and process */
    if (fgets(input, 64 , stdin) != NULL && strcmp(input,"\n") != 0)
  {
    int len = strlen(input);
    input[len - 1] = '\0';

    //nullify the arguments before each command
    for(int i = 0; i < 10; i++){
      args[i] = NULL;
    }

    //use strtok to generate the tokens
   
    char *argument = strtok(input, " "); //first should be the command, next are the args
    int i = 0;
    while(argument != NULL){ //fill up arguments with args
      args[i] = argument;
      argument = strtok(NULL, " ");  
      i++;
    }// now args is full of our arguments


    /* check for each built in command and implement */

     /*  else  program to exec */
     if(strcmp(args[0],"where")==0){
       where(args[1],pathlist); 
     }
     else if(strcmp(args[0],"which")==0){
       which(args[1],pathlist); 
     }
       /* find it */
       /* do fork(), execve() and waitpid() */
     else if(strcmp(args[0],"pwd")==0){
       printWorkingDir();
     } 
     else if(strcmp(args[0],"exit")==0){
       return 0;
     }
     else if(strcmp(args[0],"cd")==0){
       changeDir(args);
     }
     else if(strcmp(args[0],"list")==0){
       if(args[1] == NULL){
       list(getcwd(NULL, 0));
       }
       else{
        list(args[1]);
       }
     }
     else if(strcmp(args[0],"pid")==0){
       printPid();
     }

     else{
        //char *commandPath = which(args[0],pathlist);
        int PID = fork();
        if(PID == 0){ //child
          char *newenviron[] = { (char*)0 };
          execve("/usr/bin/ls",args,newenviron);
          
        }
        else{ //parent
          wait(NULL);
          printf("child finished");
        }

        //fprintf(stderr, "%s: Command not found.\n", args[0]);
    }
  }
  else if(input != NULL && strcmp(input,"\n") == 0){
    //do nothing, the user just hit a newline
  }
  else{
    printf("Error collecting input\n");
  }
  }
  return 0;
} /* sh() */


char *which(char *command, struct pathelement *p) //NOTE: SEG FAULTING HERE FOR SOME REASON
{
  char cmd[64];
  while (p) {         // WHERE
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, X_OK) == 0)
      printf("%s\n", cmd);
    p = p->next;
  }
} /* which() */

char *where(char *command, struct pathelement *p )
{
  char cmd[64];
  while (p) {         // WHERE
    char str[128];
    strcpy(str,"%s/");
    strcat(str,command);
    sprintf(cmd, str, p->element);
    if (access(cmd, X_OK) == 0)
      printf("%s\n", cmd);
    p = p->next;
  }
} /* where() */

void printWorkingDir(){
  char *ptr;

  ptr = getcwd(NULL, 0);

  printf("[%s]\n", ptr); 

  free(ptr);  // otherwise, there is memory leak...
}

void changeDir(char**args ){
         if(args[1] == NULL){
         if(chdir(getenv("HOME")) != 0){
            printf("Error locating home directory.\n");
         }
       }
       else if(args[1] == "-"){
         if(chdir("..") != 0)
            printf("Error changing Directory.\n");
       }
       else{
         if(chdir(args[1])!= 0){
           printf("Argument is not a directory or does not exist.\n");
         }
       }
}

void list(char* dir){
  struct dirent *file;
  DIR* directory = opendir(dir);
  while((file = readdir(directory)) != NULL){
        if(*file->d_name != '.'){ //ignore current dir and last dir along with any hidden files
                printf("%s\n",file->d_name); //print the file/dir name
        }
        
    }
}

void printPid(){
  printf("%d\n",getpid());
}



