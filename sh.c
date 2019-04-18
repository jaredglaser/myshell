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
#include <glob.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <utmpx.h>

node *head = NULL; //for watchmail
nodeu *headu = NULL; //for watchuser
pthread_mutex_t watchusert = PTHREAD_MUTEX_INITIALIZER;
pthread_t threadu = 0;
int watchingUser = 0;
int zombieIDs[40];

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char input [64];

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
    /*handle removing zombies*/
    for(int i=0; i<40;i++){
      if(zombieIDs[i] != NULL){
      //printf("Waiting for %d",zombieIDs[i]);
      if(waitpid(zombieIDs[i], &status, WNOHANG)!=0){
        zombieIDs[i] = NULL;
      } 
      }

    }


    /* print your prompt */
    //generate directory
    char *workingdir;
    //calloc the args array
    char **args = calloc(MAXARGS, sizeof(char*));
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
    if (fgets(input, 64 , stdin) != NULL){
    if(strcmp(input,"\n") != 0){
    int len = strlen(input);
    input[len - 1] = '\0';
    char *copy = (char*)malloc((strlen(input)+1)*sizeof(char));
    strcpy(copy,input);
    //use strtok to generate the tokens
    char *argument = strtok(input, " "); //first should be the command, next are the args
    int i = 0;
    while(argument != NULL){ //fill up arguments with args
      args[i] = argument;
      argument = strtok(NULL, " ");  
      i++;
    }// now args is full of our arguments

    /* check for each built in command and implement */
      if(isBuiltin(args[0])){
      printf("Executing built-in [%s]\n",args[0]);
      if(strcmp(args[0],"exit")==0){
        //free fields
        free(copy);
        free(prompt);
        free(commandline);
        free(owd);
        //nodeu *tmpu;
        if(headu != NULL){ //handle if never watched user
        
        while (headu != NULL){
          nodeu *tmpu = headu;;
          free(tmpu->data);
          free(tmpu);
          headu = headu->next;
        }
        }
        if(head != NULL){ //handle if never watched mail 
        
        while (head != NULL){
          node *tmp = head;
          free(tmp->data);
          free(tmp);
          head = head->next;
        }
        }
        while(pathlist->next != NULL) {
          struct pathelement* next = pathlist->next;
          free(pathlist);
          pathlist = next;
        }
        
        free(pathlist);

        free(args);

        if(threadu != 0){
          free(threadu);
        }
      
        return 0;
     }
     else{
        
        builtIns(args,pathlist,prompt,envp);
     }
      }
     /*  else  program to exec */
     else{
       forkit(args, envp,pathlist,copy,i,prompt);

    }
    free(copy);
  }
  else{
    //it was a newline so we do nothing
  }
  }
  else{
      printf("\n");
      clearerr(stdin);
      //control + D was pressed
  }
      free(args);
  }

  return 0;
} /* sh() */


void *which(char *command, struct pathelement *p)
{
  char cmd[64];
  while (p) {         // WHERE
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, X_OK) == 0){
      printf("%s\n", cmd);
      break;
    }
    p = p->next;
  }
} /* which() */

char* whichRet(char *command, struct pathelement *p)
{
  char cmd[256];
  while (p) {         // WHERE
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, X_OK) == 0){
      char *ret = (char*)malloc((strlen(cmd)+1)*sizeof(char));
      strcpy(ret,cmd);
      return ret;
    }
     
    p = p->next;
  }
  return NULL;

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
       else if(strcmp(args[1],"-") == 0){
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
      if(args[i] != NULL){ //allow default list behavior
      printf("\n%s:\n",args[i]);
      }
      while((file = readdir(directory)) != NULL){
        if(*file->d_name != '.'){ //ignore current dir and last dir along with any hidden files
                printf("%s\n",file->d_name); //print the file/dir name
        }
        
      }
      
    }
    else{ //error when opening the dir earlier
      
      printf("error opening %s\n", args[i]);
    }

    if(args[i] == NULL) //we malloced cwd
      free(cwd);
    if(closedir(directory) == -1){
      perror("closedir");
    }
    i++;
}
}

void printPid(){
  printf("%d\n",getpid());
}

void killProc(char** args){
  if(args[1]!= NULL && args[2]!= NULL){ //we have a flag
    if(strstr(args[1],"-")){ //if 1 is the one with the flag
      
      if(kill(atoi(args[2]),atoi(args[1]+1))==-1){
        perror("Kill error");
      }
    }
    else{// 2 is the flag
      if(kill(atoi(args[1]),atoi(args[2]+1))==-1){
        perror("Kill error");
      }
    }
  }
  else if(args[1] != NULL){ //no sig given
    if(kill(atoi(args[1]),SIGTERM)==-1){
      perror("Kill error");
    }

  }
  else{
    printf("kill: not enough arguments.\n");
  }
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

void printenv(char **envp, char **args,int forcePrintAll){
  if(args[1] != NULL && forcePrintAll == 0){
    char* res = getenv(args[1]);
    if(res != NULL)
    printf("%s\n",res);
    else
    printf("Does not exist.\n");
    
  }
  else{
  while (*envp){
    printf("%s\n", *envp++);
  }
  }
}

void setEnviornment(char **envp, char**args,struct pathelement *pathlist){
  if(args[1] == NULL){ //given no args
    printenv(envp,args,1);
  }
  else if(args[1] != NULL && args[2] == NULL){
    
    if(setenv(args[1],"",1)==-1){
      perror("Set Enviornment error");
      return;
    }
    if(strcmp(args[1],"HOME")==0){
      while(pathlist->next != NULL) {
        struct pathelement* next = pathlist->next;
        free(pathlist);
        pathlist = next;
      }
      free(pathlist);

      pathlist = get_path();
    }
  }
  else if(args[1] != NULL && args[2] != NULL){
    if(setenv(args[1],args[2],1)==-1){
      perror("Set Enviornment error");
      return;
    }
  }
  else{
    printf("setenv: Too many arguments.\n");
  }
}

void forkit(char**o_args, char **envp,struct pathelement *pathlist,char*copy, int numArgs, char* prompt){
        
        int offset = 0;
        int operator = 0;
        int fd;
        int leftside = 1;
        int wasBackround = 0;
        int pipefd[2];
        int numArgsPipe = 0;
        if(strcmp(o_args[numArgs-1],"&")==0){
            operator = -1;
            wasBackround = 1;
        }
        for(int i=0; i<numArgs; i++){ //look for a > or whatev
          if(strcmp(o_args[i],">") ==0){
            offset = i;
            operator = 1; 
          }
          if(strcmp(o_args[i],">>") ==0){
            offset = i;
            operator = 3; 
          }
          if(strcmp(o_args[i],">&") ==0){
            offset = i;
            operator = 2; 
          }
          if(strcmp(o_args[i],">>&") ==0){
            offset = i;
            operator = 4; 
          }
          if(strcmp(o_args[i],"<") ==0){
            offset = i; // file < command
            operator = 5; 
          }
          if(strcmp(o_args[i], "|") ==0){
            offset = i;
            operator = 6;
          }

        }
        char** newArgs;
        
        if(operator != 0){
            //apply neccessary commands to get the input and output working
          if(operator == 1 || operator == 2 || operator == 3 || operator ==4){ // > or >&
              numArgs = offset; //offset will be after the args so args will be index 0 - offset-1
              offset = 0;
          }
          else if(operator == 5){
            numArgs = numArgs - (offset + 1); // file < arg1 arg2 arg3
            offset = 2;
          }
          else if(operator == -1){
            numArgs = numArgs-1;
            offset = numArgs;
            leftside = 0;
          }
          else if(operator == 6){ //command flag | command flag
            numArgsPipe = numArgs - offset - 1;
            numArgs = offset;
            offset = 0;
          }
        }
        
        
        
        char **args = calloc(MAXARGS, sizeof(char*));
        char **argsPipe = calloc(MAXARGS, sizeof(char*));
        
        if(leftside){
          int j = 0;
        for(int i=offset;i<numArgs+offset;i++){
          args[j] = o_args[i];
          j++;
        }
        }
        else{
          int j = offset-1;
          for(int i=offset-1;i>=0;i--){
          args[j] = o_args[i];
          j--;
        }
        }

        if(operator == 6){ //pipe needs pipe array filled
          for(int i =0; i<numArgsPipe; i++){
            argsPipe[i] = o_args[numArgs+1+i];
          }
        }


        //TESTING
        for(int i=0;i<numArgs;i++){
          printf("[%d] %s\n",i,args[i]);
        }

        if(operator == 6){
          for(int i=0;i<numArgs;i++){
          printf("[%d] %s\n",i,argsPipe[i]);
        }
        }
        
        if(operator !=6){
        int PID = fork();
        int status;
        char * res;
        if(PID == 0){ //child
        if(strstr(args[0],"./") != NULL || strstr(args[0],"../") != NULL || args[0][0] == '/' || whichRet(args[0],pathlist)){
          int fd;
          if(operator == 1){
            //handle case with >
            if(close(STDOUT_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
              dup(fd);
              close(fd);
            }
          }
          else if(operator == 2){
            //handle >&
            if(close(STDOUT_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
            dup(fd);
            close(fd);
            }
            if(close(STDERR_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_APPEND|O_WRONLY|O_TRUNC,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
              dup(fd);
              close(fd);
            }
          }
          else if(operator ==3){
            //handle case with >>
            if(close(STDOUT_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_CREAT|O_WRONLY|O_APPEND,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
              dup(fd);
              close(fd);
            }
          }
          else if(operator == 4){
            //handle >>&
            if(close(STDOUT_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_CREAT|O_WRONLY|O_APPEND,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
            dup(fd);
            close(fd);
            }
            if(close(STDERR_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[numArgs+1],O_WRONLY|O_APPEND,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
              dup(fd);
              close(fd);
            }
          }
          else if(operator == 5){
            //handle case with <
            if(close(STDOUT_FILENO)==-1){
              perror("Close Error");
            }
            if(fd = open(o_args[0],O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU)==-1){
              perror("Open Error");
            }
            else{
              dup(fd);
              close(fd);
            }
          }
        }

          int isWild = 0;
          glob_t globbuf;
          int gl_offs_count = 0;
          if(strstr(copy,"*") || strstr(copy,"?")){
            //there are wildcards
            isWild = 1;
            //find any flags
            int flag = 1;
            for(int i = 1; i<MAXARGS; i++){
              if(args[i] == NULL)
                break;
              if((strstr(args[i],"-"))){ //a wildcard
                flag++;
              }
            }
            globbuf.gl_offs = flag;

            for(int i = 1; i<MAXARGS; i++){
              if(args[i] == NULL)
                break;
              if((strstr(args[i],"*")) || (strstr(args[i],"?"))){ //a wildcard
                gl_offs_count++; //add up all the numbers of wildcards
              }
            }
            //go through again but this time globbing 
            int first = 0;
            for(int i = 1; i<MAXARGS; i++){
              if(args[i] == NULL)
                break;
              if((strstr(args[i],"*")) || (strstr(args[i],"?"))){ //a wildcard
                if(first)
                glob(args[i], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
                else
                glob(args[i], GLOB_DOOFFS, NULL, &globbuf);
              }
            }
            globbuf.gl_pathv[0] = args[0]; //set pathv[0] to the command name
            
            //set the rest
            
            for(int i = 1; i<MAXARGS; i++){
              if(args[i] == NULL)
                break;
              if((strstr(args[i],"-"))){ //a wildcard
                globbuf.gl_pathv[i] = args[i];
              }
            }


          } 
         
          
          //if the arg contains ./ ../ or starts with a /
          if(strstr(args[0],"./") != NULL || strstr(args[0],"../") != NULL || args[0][0] == '/'){ 
            if (access(args[0], X_OK) == 0){ //if the path given is to an executable
              printf("Executing [%s]\n",args[0]);
              if(execve(args[0],args,envp)==-1){
                perror("exec");
              }
            }
            else{
              printf("Executable not found.\n");
            }
          }
          /*
          // If the command is in the path
          */
          else if(res = whichRet(args[0],pathlist)){
            printf("Executing [%s]\n",args[0]);

            if(isWild){
            
              if(execvp(args[0], &globbuf.gl_pathv[0])==-1){
                perror("exec");
              }
            }
            else{
            if(execve(res,args,envp)==-1){
              perror("exec");
            }
            }
            free(res);
            
          }
          /*
          // If the command is not in the path
          */
          else{
            printf("Command not found\n");
          }
          
        }
        else{ //parent
          if(wasBackround){
          //add to the backround array
          int added=0;
          for(int i=0; i< 40; i++){
            if(zombieIDs[i]== NULL){
              //printf("Added %d",PID);
              zombieIDs[i] = PID;
              added=1;
              break;
            }
          }
          if(!added){
            printf("Too many backround processes.\n");
          }
          }
          else
          waitpid(-1, &status, 0);

          if(operator != 0){
              fd = open("/dev/tty", O_WRONLY);
            }

        }
          
        }
        else{ //pipe implementation
          pid_t pid = fork();
          if(pid > 0){ //parent
            int status;
            waitpid(pid, &status, 0);
          }
          else{//child
             int pipeStatus;
              int pfds[2];
              pipe(pfds);
              pid_t first = fork();
              // first process / parent
              if (first) {
                /* char* c = which(com, pathlist); */
                close(1);
                dup(pfds[1]);
                close(pfds[0]);
                if(!isBuiltin(args[0])){
                  if (-1 == execve(whichRet(args[0],pathlist), args, envp)) {
                    perror(args[0]);
                  }
                }
                else{
                  builtIns(args,pathlist,prompt,envp);
                }
                close(pfds[1]);
              }
              else {
                close(0);
                dup(pfds[0]);
                close(pfds[1]);
                execve(whichRet(argsPipe[0],pathlist), argsPipe, envp);
                waitpid(first, &pipeStatus, 0);
              }
            }
          }

        //handle frees
        free(args);
        free(argsPipe);
        

        //fprintf(stderr, "%s: Command not found.\n", args[0]);
}

void addnode(pthread_t thread, char *data) {
    node *tmp = malloc(sizeof(node));
    tmp->thread = thread;
    tmp->data = malloc(sizeof(data+1));
    strcpy(tmp->data,data);
    node *headref = head;
    tmp->next = NULL;
    if(!head){
      head = tmp;
      return;
    }
    while(headref->next != NULL){
      headref = headref->next;
    }
    headref->next = tmp;
}
void addnodeu(int thread, char *data) {
    nodeu *tmp = malloc(sizeof(nodeu));
    tmp->isLogged = thread;
    tmp->data = malloc(sizeof(data+1));
    strcpy(tmp->data,data);
    nodeu *headref = headu;
    tmp->next = NULL;
    if(!headu){
      headu = tmp;
      return;
    }
    while(headref->next != NULL){
      headref = headref->next;
    }
    headref->next = tmp;
}

void watchmail(char **args){
  pthread_t thread;
  if(args[2]!= NULL){if(!strcmp(args[2], "off")){ //it can take an optional second argument of "off" to turn off of watching of mails for that file.
    node *tmp = head;
    node *prev;
    if(tmp != NULL && !strcmp(tmp->data, args[1])){
      pthread_cancel(tmp->thread);
      head = tmp->next;
      free(tmp->data);
      free(tmp);
      return;
    }
    while(tmp->next != NULL){
      if(!strcmp(tmp->data, args[1])){ //kill the thread
        printf("Cancelling thread %d\n", tmp->thread); //delete node...
        pthread_cancel(tmp->thread);
        //Cannot free since it might break the list. Free all on exit.
      }
      prev = tmp;
      tmp = tmp->next;
    }
    prev->next = tmp->next;
  }}
  else{ //actually watch the file with pthread_create(3) 
    if(pthread_create(&thread, NULL, &watchmailthread, args)) { //on success, returns 0
      fprintf(stderr, "Error creating thread\n");
    }
    //printf("THE THREAD IS: %d\n", thread);
    addnode(thread, args[1]); //append the thread ID to the linked list.
  }
}
void *watchmailthread(char **args){
  struct stat sb;
  struct stat sb1;
  if(stat(args[1], &sb)){
    printf("Error getting file information\n");
  }
  int size = sb.st_size;
  while(1){ //A sleep for 1 second should be in the loop
    sleep(1);
    if(!stat(args[1], &sb1)){
      if(sb1.st_size != size){
        struct timeval tv;
        time_t timenow;
        struct tm *nowtm;
        char tmbuf[64], buf[64];
        gettimeofday(&tv, NULL);
        timenow = tv.tv_sec;
        nowtm = localtime(&timenow);
        strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
        printf("\a\n BEEP You've Got Mail in %s at %s \n", args[1], tmbuf);
        size = sb1.st_size;
      }
    }
  }
}
void watchuser(char **args){
//The first time watchuser is ran, a (new) thread should be created/started via pthread_create(3)
  if(watchingUser==0){
    watchingUser = 1;
    if(pthread_create(&threadu, NULL, &watchuserthread, args)) { //same as watchmail basically.
        fprintf(stderr, "Error creating thread\n");
    }
  }  
  if(args[2] == NULL){ //INITIALIZE THE LINKED LIST / ADD TO LINKED LIST
    pthread_mutex_lock(&watchusert);
    addnodeu(0, args[1]);
    pthread_mutex_unlock(&watchusert);
    nodeu *looper = headu;
    int looppos = 0;
    while(looper != NULL){
      printf("[%d] %s\n",looppos, looper->data);
      looppos++;
      looper = looper->next;
    }
  }
  else if(args[2] && strcmp(args[2],"off") == 0){ //"off" implementation
    nodeu *tmp = headu;
    nodeu *prev = headu;
    if(!strcmp(args[1], tmp->data)){
      headu = tmp->next;
      printf("Freeing username IN HEAD:  %s\n", tmp->data);
      free(tmp->data);
      free(tmp);
    }
    else{
      while(tmp!=NULL && strcmp(tmp->data, args[1])){
          prev = tmp;
          tmp = tmp->next;        
      }
          prev->next = tmp->next;
          printf("freeing %s\n", tmp->data);
          free(tmp->data);
          free(tmp);
    }
  }
  else{
    printf("Invalid arguments for watchuser.\n");
  }
}
//The thread should get the list of users from a global linked list which the calling function
//(of the main thread) will modify by either inserting new users or turning off existing watched users.
void *watchuserthread(char **args){
  while(1){
    nodeu *tmp = headu;
    while(tmp){
      if(!tmp->isLogged){
        struct utmpx *up;
        setutxent();
        while (up = getutxent()){   /* get an entry */
          if (up->ut_type == USER_PROCESS && !strcmp(tmp->data, up->ut_user)){  /* only care about users */
            printf("%s has logged on %s from %s\n", up->ut_user, up->ut_line, up ->ut_host);
            
            tmp->isLogged = 1; //This allows for a check later to see current users. Do this for all users.
          }
          
        }
      }
      else{
          //printf("Checking: %s\n",tmp->data);
          struct utmpx *up;
          setutxent();
          int didLeave = 1;
          while (up = getutxent()){   /* get an entry */
          if (up->ut_type == USER_PROCESS){  /* only care about users */
            nodeu *usr = headu;
            while(usr != NULL){
              if(strcmp(up->ut_user,tmp->data) == 0){
                //printf("Match Found\n");
                didLeave = 0;
                break;
              }
              //printf("No Match Found\n");
              usr = usr->next;
            }
            if(didLeave == 0){
              //printf("Match Found\n");
              break;
            }
            
          }
          
        }
        if(didLeave){
            printf("%s has logged out\n", tmp->data);
            tmp->isLogged = 0; //This allows for a check later to see current users. Do this for all users.
        }
        else{
          //printf("%s is still logged in\n",tmp->data);
      }
      }
      tmp = tmp->next;
    
  }
  sleep(3);
}
}

void builtIns(char**args, struct pathelement* pathlist,char*prompt, char** envp){
  if(strcmp(args[0],"where")==0){
       if(args[1] != NULL)
       for(int i = 0; i<MAXARGS; i ++){
         if(args[i] != NULL)
          where(args[i],pathlist);
        else
          break; 
       }
       else
       printf("Where: not enough arguments\n");
     }
     else if(strcmp(args[0],"which")==0){
       if(args[1] != NULL){
       for(int i = 1; i<MAXARGS; i ++){
         if(args[i] != NULL)
          which(args[i],pathlist); 
        else
          break; 
       }
       }
       else{
         printf("Which: not enough arguments\n");
       }
     }
       /* find it */
       /* do fork(), execve() and waitpid() */
     else if(strcmp(args[0],"pwd")==0){
       if(args[1] != NULL)
       printf("pwd: too many arguments \n");
       else
       printWorkingDir();
     } 
    else if(strcmp(args[0],"cd")==0){
      if(args[2] != NULL){
        printf("cd: too many arguments\n");
      }
      else{
       changeDir(args);
      }
    }
     else if(strcmp(args[0],"list")==0){
       list(args);
     }
     else if(strcmp(args[0],"watchmail")==0){
        watchmail(args);
     }
     else if(strcmp(args[0],"watchuser")==0){
        watchuser(args);
     }
     else if(strcmp(args[0],"pid")==0){
       if(args[1] != NULL)
       printf("pid: too many arguments\n");
       else
       printPid();
     }
     else if(strcmp(args[0],"prompt")==0){
       promptCmd(args, prompt);
     }
     else if(strcmp(args[0], "kill")==0){
       if(args[3]!=NULL)
       printf("kill: too many arguments\n");
       else
       killProc(args);
     }
      else if(strcmp(args[0], "printenv")==0){
       printenv(envp,args,0);
     }
      else if(strcmp(args[0], "setenv")==0){
        if(args[3] != NULL){
          printf("setenv: too many arguments\n");
        }
        else
       setEnviornment(envp,args,pathlist);
     }
}

int isBuiltin(char* command){
  if((strcmp(command,"where")==0)|| //internal command
      (strcmp(command,"which")==0)||
      (strcmp(command,"watchmail")==0)||
      (strcmp(command,"watchuser")==0)||
      (strcmp(command,"pwd")==0)||
      (strcmp(command,"exit")==0)||
      (strcmp(command,"cd")==0)||
      (strcmp(command,"list")==0)||
      (strcmp(command,"pid")==0)||
      (strcmp(command,"prompt")==0) ||
      (strcmp(command,"kill")==0) ||
      (strcmp(command,"printenv")==0) ||
      (strcmp(command,"setenv")==0) ){
        return 1;
  }
  else{
    return 0;
  }
}