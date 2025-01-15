#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>

void f(int n){
  if(n<2){
    printf("Hi... n=%d from %d parent=%d\n",n,getpid(),getppid());
    return;
  }
  else{

    if(!fork()){
      f(n-1);
    }
    if(!fork()){
      f(n-2);
    }
  }
}

int main ( int argc, char *argv[] )
{
int n;
if (argc == 1) exit(1);
n = atoi(argv[1]);
f(n);
exit(0);
}
