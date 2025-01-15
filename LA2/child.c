#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<time.h>
#define MAXLINE 1024
#define MAXCHILD 1024
#define PLAYING 0
#define CATCH 1
#define MISS 2
#define OUTOFGAME 3

int n;
int my_number;
int my_status;
int start;

pid_t childs[1024];



void catch_or_miss(int sig){
  int r=rand()%10;
  if(r<=7){
    my_status=CATCH;
    kill(getppid(),SIGUSR1);
  }else{
    my_status=MISS;
    kill(getppid(),SIGUSR2);
  }
}

void print() {
    if (my_status == PLAYING) {
        printf("    ....   ");
    } else if (my_status == CATCH) {
        printf("   CATCH   ");
    } else if (my_status == MISS) {
        printf("   MISS    ");
    } else if (my_status == OUTOFGAME) {
        printf("           "); 
    }
}

void init(){
  if(start==1 && my_number==1){
    printf("\n");
    for(int i=1;i<=n;i++){
      if(i==1)printf("      %d      ", i);
      else printf("     %d      ", i);
    }
    printf("\n");
    start=0;
  }
}

void print_status(int sig){
  init();
  if(my_number==1){
    for (int i = 0; i < n; i++) {
        if(i==0)printf("------------+");
        else printf("-----------+");
    }
    printf("\n|");
  }

  //printf("Child%d: ",my_number);
  fflush(stdout);
  print();
  printf("|");
  fflush(stdout);
  
  
  if(my_status==CATCH){
    my_status=PLAYING;
  }
  else if(my_status==MISS){
    my_status=OUTOFGAME;
  }
  if(my_number!=n){
    pid_t npid=childs[my_number];
    kill(npid,SIGUSR1);
  }else{
    printf("\n");
    FILE *fpdummy=fopen("dummycpid.txt","r");
    pid_t dpid;
    fscanf(fpdummy,"%d",&dpid);
    fclose(fpdummy);
    kill(dpid,SIGINT);
  }
}

int main(){
  srand(time(NULL)+getpid());
  sleep(1);
  FILE *fp=fopen("childpid.txt","r");
  fscanf(fp,"%d",&n);
  //printf("Total number of children: %d\n",n);
  pid_t mypid=getpid();
  start=1;
  
  for(int i=0;i<n;i++){
    pid_t pid;
    fscanf(fp,"%d",&pid);
    //printf("pid=%d\n",pid);
    if(pid==mypid){
      //printf("I'm child%d with PID=%d\n",i+1,pid);
      my_number=i+1;
      my_status=PLAYING;
    }
    childs[i]=pid;
  }
  fclose(fp);
  signal(SIGUSR1,print_status);
  signal(SIGUSR2,catch_or_miss);
  while(1)pause();

  exit(0);
}
