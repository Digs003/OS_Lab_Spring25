#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>


void f(int n){
  if(n<2){
    printf("Hi...pid=%d\n",getpid());
   // exit(0);
  }else{
    if(!fork())f(n-1);
    if(!fork())f(n-2);
  }
}
int main(){
  f(3);
  return 0;
}
