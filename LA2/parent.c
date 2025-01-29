#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<signal.h>
#include<time.h>
#define MAXCHILD 1024
#define PLAYING 0
#define CATCH 1
#define MISS 2
#define OUTOFGAME 3

int current_player;
int num_players;
struct child_type{
  pid_t pid;
  int status;
};
struct child_type children[MAXCHILD];

void handler_1(int sig){  
  //children[current_player].status=CATCH;
}

void handler_2(int sig){
  children[current_player].status=MISS;
  num_players--;
}

void handle_print(){
  pid_t dpid=fork();
  if(dpid==-1){
    fprintf(stderr,"Error forking dummy node\n");
    exit(1);
  }
  else if(dpid==0){
    execl("./dummy","./dummy",NULL);
    exit(0);
  }else{
    FILE * fpdummy=fopen("dummycpid.txt","w");
    fprintf(fpdummy,"%d\n",dpid);
    fclose(fpdummy);
    kill(children[0].pid,SIGUSR1);
    waitpid(dpid,NULL,0);
  }
}

int main(int argc,char* argv[]){
  srand(time(NULL));
  if(argc<2){
    printf("Specify number of children\n");
    exit(1);
  }
  int n=atoi(argv[1]);
 
  num_players=n;

  FILE* fp=fopen("childpid.txt","w");
  if(fp==NULL){
    fprintf(stderr,"Error opening file\n");
    exit(1);
  }
  fprintf(fp,"%d\n",n);
  
  for(int i=0;i<n;i++){
    pid_t pid=fork();
    if(pid==-1){
      fprintf(stderr,"Error forking\n");
      exit(1);
    }else if(pid==0){
      execl("./child","./child",NULL);
      exit(0);
    }else{
      children[i].pid=pid;
      children[i].status=PLAYING;
      fprintf(fp,"%d\n",pid);
    }
  }
  fclose(fp);
  printf("Parent: %d child processes created\n",n);
  printf("Parent: Waiting for child processes to read child database\n");
  sleep(2);
  signal(SIGUSR1,handler_1);
  signal(SIGUSR2,handler_2);
  current_player=0;
  while(1){
    if(num_players==1){
      break;
    }
    if(children[current_player].status==PLAYING){
      kill(children[current_player].pid,SIGUSR2);
      handle_print();
    }
    current_player=(current_player+1)%n;
  }
  for(int i=0;i<n;i++){
    kill(children[i].pid,SIGINT);
  }

  exit(0);
}





