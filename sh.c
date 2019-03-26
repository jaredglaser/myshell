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
    free(pwd);
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';
  free(pwd); //malloc'd by getcwd
  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */

    //generate directory
    char *workingdir;
    
    workingdir =getcwd(NULL, 0);
    if(strcmp(prompt," ") == 0){ //ignore the prompt if it is the default space
      printf("[%s]",workingdir);
    }
    else{
    printf("%s [%s]",prompt,workingdir);
    }
    free(workingdir);//malloc'd by getcwd
    printf(">>");
    /* get command line and process */
    if (fgets(input, 64 , stdin) != NULL && strcmp(input,"\n") != 0)
  {
    int len = strlen(input);
    input[len - 1] = '\0';

    //nullify the arguments before each command
    for(int i = 0; i < 64; i++){  //expecting only 64 max args. May see issues if user exceeds this.
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
       //free fields
       free(prompt);
       free(commandline);
       free(owd);
       

       while(pathlist->next != NULL){ //free pathlist which was malloc'd by get_path
         struct pathelement *p = pathlist;
         pathlist = pathlist->next;
         if(p->element != NULL)
          free(p->element);
         free(p);
       } 
       if(pathlist->element != NULL)
        free(pathlist->element);
       free(pathlist);

       for(int i=0; i < MAXARGS; i++)
          free(args[i]);
        free(args);
       return 0;
     }
     else if(strcmp(args[0],"cd")==0){
       changeDir(args);
     }
     else if(strcmp(args[0],"list")==0){
       list(args);
     }
     else if(strcmp(args[0],"pid")==0){
       printPid();
     }
     else if(strcmp(args[0],"prompt")==0){
       promptCmd(args, prompt);
     }

     else{
        
        int PID = fork();
        int status;
        if(PID == 0){ //child

          
          char *newenviron[] = { (char*)0 };
          
          //if the arg contains ./ ../ or starts with a /
          if(strstr(args[0],"./") != NULL || strstr(args[0],"../") != NULL || args[0][0] == '/'){ 
            if (access(args[0], X_OK) == 0) //if the path given is to an executable
              execve(args[0],args,newenviron);
            else{
              printf("Executable not found.\n");
            }
          }
          /*
          // If the command is in the path
          */
          else if(which(args[0],pathlist) != NULL){
            execve(which(args[0],pathlist),args,newenviron);
            
          }
          /*
          // If the command is not in the path
          */
          else{
            printf("Command not found\n");
          }
          
        }
        else{ //parent
          waitpid(-1, &status, 0);
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

void list(char **args){
  struct dirent *file;
  
  int i = 1;
  while(1){
    DIR* directory;
    char* cwd;
    if(i == 1 && args[i] == NULL){
    
    directory = opendir(cwd = getcwd(NULL, 0));
    
    }
    else if(args[i] != NULL){
      directory = opendir(args[i]);
    }
    else{
      
      break;
    }
    if(directory != NULL){
      printf("\n%s:\n",args[i]);
      while((file = readdir(directory)) != NULL){
        if(*file->d_name != '.'){ //ignore current dir and last dir along with any hidden files
                printf("%s\n",file->d_name); //print the file/dir name
        }
        
      }
      
    }
    else{ //error when opening the dir earlier
      
      printf("error opening %s", args[i]);
    }

    if(args[i] == NULL) //we malloced cwd
      free(cwd);
    closedir(directory);
    i++;
}
}

void printPid(){
  printf("%d\n",getpid());
}

void promptCmd(char** args, char* prompt){
  if(args[1] != NULL){
    //prompt is given
    strcpy(prompt,args[1]);
  }
  else{
    char input[64];
    printf("input prompt prefix:");
    while(1){
    if (fgets(input, 64 , stdin) != NULL && strcmp(input,"\n") != 0)
    {
      int len = strlen(input);
      input[len - 1] = '\0';
      strcpy(prompt,input);
      break;
    }
    }
}
}



