#include <stdio.h>
#include <unistd.h>

int main( int argc, char **argv, char **envp )
{
    while(1){
    printf("Hello from infinite process\n");
    sleep(2);
    }

}
