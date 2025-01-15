#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
int main(){
  int status;
  if(fork()==0){
    exit(1);
  }else{
    wait(&status);
  }
  if(WIFEXITED(status)){
    printf("Child exited with code %d\n", WEXITSTATUS(status));
  }else if(WIFSIGNALED(status)){
    psignal(WTERMSIG(status),"Exit signal");
  }
  return 0;
}
