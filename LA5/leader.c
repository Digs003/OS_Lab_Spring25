#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define TABLE_SIZE 1024
int sum_hash[TABLE_SIZE];


int hash_insert(int value) {
    int index = value % TABLE_SIZE;
    while (sum_hash[index] != 0) {
        if (sum_hash[index] == value) {
            return 1;  
        }
        index = (index + 1) % TABLE_SIZE; 
    }
    sum_hash[index] = value; 
    return 0; 
}

int main(int argc,char* argv[]){
  if(argc>2){
    printf("Too many arguments \n");
    exit(1);
  }
  int n=10;
  if(argc==2){
    n=atoi(argv[1]);
  }
  int shmid=shmget(ftok("/",'A'),(n+4)*sizeof(int),0777|IPC_CREAT|IPC_EXCL);
  if(shmid==-1){
    fprintf(stderr,"ERROR:Leader already present\n");
    exit(1);
  }
  int* M=(int*)shmat(shmid,0,0);
  M[0]=n;
  M[1]=0;
  M[2]=0;
  int start=1;
  printf("Waiting for %d followers...\n",n);
  while(M[1]!=M[0]){}
  while(1){
    if(M[2]==0){
      if(start==0){
        int curr_sm=0;
        for(int i=3;i<n+4;i++){
          if(i==n+3) printf("%2d = ",M[i]);
          else printf("%2d + ",M[i]);
          curr_sm+=M[i];
        }
        printf("%2d\n",curr_sm);
        if(hash_insert(curr_sm)){
            M[2]=-1;
            break;
        }
      }
      M[3]=rand()%99+1;
      M[2]=1;
      start=0;
    }
  }
  while(M[2]!=0){}
  shmdt(M);
  shmctl(shmid,IPC_RMID,0);
  return 0;
}
