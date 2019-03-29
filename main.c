#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal); 

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */
  if(signal(SIGINT,sig_handler)==SIG_ERR){
    perror("Signal Error");
  }
  if(signal(SIGTSTP,sig_handler)==SIG_ERR){
    perror("Signal Error");
  }
  if(signal(SIGTERM,sig_handler)==SIG_ERR){
    perror("Signal Error");
  }


  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  if(signal == SIGINT){
    printf("\n");
  }
  

}

