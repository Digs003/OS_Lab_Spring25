#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/types.h>
#include<unistd.h>
#include<signal.h>
#include<time.h>

#define SELF 0
#define RUNNING 1
#define FINISHED 2
#define TERMINATED 3
#define SUSPENDED 4
#define KILLED 5
#define MAXCHILD 10
struct procinfo{
  pid_t pid;
  pid_t pgid;
  int state;
  char arg;
};

struct procinfo PT[MAXCHILD+1];
int nchild,runningchild,termination_type;

void printHelp(){
   printf("   Command : Action\n");
   printf("      c    : Continue a suspended job\n");
   printf("      h    : Print this help message\n");
   printf("      k    : Kill a suspended job\n");
   printf("      p    : Print the process table\n");
   printf("      q    : Quit\n");
   printf("      r    : Run a new job\n");
}

int runJob(struct procinfo PT[],int nchild,char arg[]){
  int pid;
  ++nchild;
  if(nchild>MAXCHILD){
    fprintf(stderr,"Process Table is full,Quitting...\n");
    exit(1);
  }
  arg[0]=rand()%26+'A';
  pid=fork();
  if(pid<0){
    fprintf(stderr,"Fork failed,Quitting...\n");
    exit(1);
  }
  else if(pid){
    PT[nchild].pid=pid;
    PT[nchild].pgid=pid;
    PT[nchild].state=RUNNING;
    PT[nchild].arg=arg[0];
    runningchild=nchild;
    termination_type=FINISHED;
    waitpid(pid,NULL,WUNTRACED);
    PT[nchild].state=termination_type;
    runningchild=0;
  }else{
    setpgid(getpid(),getpid());
    execlp("./job","job",arg,NULL);
  }
  return nchild;
}
int getJobIndex(struct procinfo PT[],int nchild){
  int suspended=0;
  printf("Suspended Jobs: ");
  for(int i=1;i<=nchild;i++){
    if(PT[i].state==SUSPENDED){
      if(suspended)printf(", ");
      printf("%d",i);
      suspended=1;
    }
  }
  if(!suspended){
    printf("None");
    return -1;
  }
  printf(" Pick One: ");
  int idx;
  scanf("%d",&idx);
  while(getchar()!='\n');
  if(idx>=1 && idx<=nchild && PT[idx].state==SUSPENDED){
    return idx;
  }
  else{
    fprintf(stderr,"Invalid Job Index\n");
    return -1;
  } 
}
void continueJob(struct procinfo PT[],int nchild){
  int idx=getJobIndex(PT,nchild);
  if(idx==-1)return;
  kill(PT[idx].pid,SIGCONT);
  PT[idx].state=RUNNING;
  runningchild=idx;
  termination_type=FINISHED;
  waitpid(PT[idx].pid,NULL,WUNTRACED);
  PT[idx].state=termination_type;
  runningchild=0;
}

void killJob(struct procinfo PT[],int nchild){
  int idx=getJobIndex(PT,nchild);
  if(idx==-1)return;
  kill(PT[idx].pid,SIGKILL);
  waitpid(PT[idx].pid,NULL,0);
  PT[idx].state=KILLED;
}

void printProcTable(struct procinfo PT[],int nchild){
  printf("NO    PID    PGID    STATUS       NAME\n");
  for(int i=0;i<=nchild;i++){
    printf("%d    %d    %d    ",i,PT[i].pid,PT[i].pgid);
    if(PT[i].state==SELF)printf("SELF     mgr\n");
    else if(PT[i].state==RUNNING)printf("RUNNING      job %c\n",PT[i].arg);
    else if(PT[i].state==FINISHED)printf("FINISHED     job %c\n",PT[i].arg);
    else if(PT[i].state==TERMINATED)printf("TERMINATED   job %c\n",PT[i].arg);
    else if(PT[i].state==SUSPENDED)printf("SUSPENDED    job %c\n",PT[i].arg);
    else if(PT[i].state==KILLED)printf("KILLED       job %c\n",PT[i].arg);

  }
}

void dealC(int sig){
  if(runningchild>0){
    termination_type=TERMINATED;  
    kill(PT[runningchild].pid,SIGINT);
    waitpid(PT[runningchild].pid,NULL,0);
    runningchild=0;
  }

}

void dealZ(int sig){
  if(runningchild>0){
    termination_type=SUSPENDED;
    kill(PT[runningchild].pid,SIGTSTP);
    runningchild=0;
  }
}

int main(){
  char cmd;
  srand((unsigned int)time(NULL));
  signal(SIGINT,dealC);
  signal(SIGTSTP,dealZ);
  PT[0].pid=getpid();
  PT[0].pgid=getpgrp();
  PT[0].state=SELF;
  char arg[2];
  nchild=0;
  runningchild=-1;
  arg[0]='A';
  arg[1]=0;//NULL Termination
  while(1){
    printf("mgr> ");
    cmd=getchar();
    if(cmd=='\n') continue;
    while(getchar()!='\n');
    if(cmd=='h' || cmd=='H')printHelp();
    else if(cmd=='r' || cmd=='R')nchild=runJob(PT,nchild,arg);
    else if(cmd=='c' || cmd=='C')continueJob(PT,nchild);
    else if(cmd=='k' || cmd=='K')killJob(PT,nchild);
    else if(cmd=='p' || cmd=='P')printProcTable(PT,nchild);
    else if(cmd=='q' || cmd=='Q')break;
    else fprintf(stderr,"Invalid Command\n");
  }
  exit(0);
}
