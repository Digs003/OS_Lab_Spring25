#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>


void f(int n){
  if(n<=0)return;
  while(n--){
    printf("PID=%d, PPID=%d, n=%d\n",getpid(),getppid(),n);
    fflush(stdout);
    if(!fork())f(n);
  }
}
int main(){
  f(1);
  return 0;
}
