#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc,char* argv[]){
  if(argc>2){
    printf("Too many arguments\n");
    exit(1);
  }
  int followers=1;
  if(argc==2){
    followers=atoi(argv[1]);
  }
  for(int i=0;i<followers;i++){
    pid_t pid=fork();
    if(pid==0){
      srand(time(NULL)^getpid());
      int shmid=shmget(ftok("/",'A'),0,0777);
      if(shmid==-1){
        fprintf(stderr,"No leader present\n");
        exit(1);
      }
      int* M=(int*)shmat(shmid,NULL,0);
      if(M[1]==M[0]){
        printf("follower error: %d followers have already joined\n",M[0]);
        exit(0);
      }
      int my_id=M[1]+1;
      M[1]++;

      printf("follower %d joins\n",my_id);
      while(1){
        if(M[2]==my_id){
          M[my_id+3]=rand()%9+1;
          if(my_id==M[0]){
            M[2]=0;
          }
          else{
            M[2]++;
          }
        }
        if(M[2]==(-1)*my_id){
          if(my_id==M[0]){
            M[2]=0;
          }else{
            M[2]--;
          }
          break;
        }
      }
      printf("follower %d leaves\n",my_id);
      shmdt(M);
      exit(0);
    }
  }
  for(int i=0;i<followers;i++){
    wait(NULL);
  }
  return 0;
}
