#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define FILENAME "customers.txt"
#define TIME 100000 //1minute=100ms=100000us

void semwait(int semid,int semnum){
  struct sembuf sb;
  sb.sem_num=semnum;
  sb.sem_op=-1;
  sb.sem_flg=0;
  semop(semid,&sb,1);
}

void semsignal(int semid,int semnum){
  struct sembuf sb;
  sb.sem_num=semnum;
  sb.sem_op=1;
  sb.sem_flg=0;
  semop(semid,&sb,1);
}
char *time_str(int time)
{
    int hour = (time / 60) + 11;
    int minute = time % 60;
    char *str = (char *)malloc(20 * sizeof(char));

    if (hour >= 12) sprintf(str, "[%02d:%02d pm]", (hour % 12) ? (hour % 12) : 12, minute);
    else sprintf(str, "[%02d:%02d am]", (hour % 12) ? (hour % 12) : 12, minute);
    return str;
}
void check_error(int result, const char* msg) {
    if (result == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void cmain(int cust_id,int time_,int cust_cnt){
  int shmkey = ftok("/", 'A');
  check_error(shmkey, "ftok");
  int shmid = shmget(shmkey, 2000, 0666);
  check_error(shmid, "shmget");
  int* M = (int*)shmat(shmid, NULL, 0);
  check_error((int)(intptr_t)M, "shmat");

  int mtxkey = ftok("/", 'B');
  check_error(mtxkey, "ftok");
  int semwaiterkey = ftok("/", 'D');
  check_error(semwaiterkey, "ftok");
  int semcustomerkey = ftok("/", 'E');
  check_error(semcustomerkey, "ftok");

  int mtxid = semget(mtxkey, 1, 0666);
  check_error(mtxid, "semget");
  int semwaiterid = semget(semwaiterkey, 5, 0666);
  check_error(semwaiterid, "semget");
  int semcustomerid = semget(semcustomerkey, 200, 0666);
  check_error(semcustomerid, "semget");

  if(time_>240){
    printf("%s\t\t\t\t\t\t Customer %d leaves (late arrival)\n",time_str(time_),cust_id);
    shmdt(M);
    exit(0);
  }
  semwait(mtxid,0);
  M[0]=time_;
  int table=M[1];
  semsignal(mtxid,0);
  if(table==0){
    printf("%s\t\t\t\t\t\t Customer %d leaves (no empty table)\n",time_str(time_),cust_id);
    shmdt(M);
    exit(0);
  }
  printf("%s Customer %d arrives (count=%d)\n",time_str(time_),cust_id,cust_cnt);

  semwait(mtxid,0);
  M[1]--;
  char waiter=M[2]+'U';
  M[2]=(M[2]+1)%5;
  int front=M[200*(waiter-'U')+102];
  if(front==0){
    front=200*(waiter-'U')+104;
    M[200*(waiter-'U')+102]=front;
    M[200*(waiter-'U')+103]=front;
  }else{
    front+=2;
    M[200*(waiter-'U')+102]=front;
  }
  M[front]=cust_id;
  M[front+1]=cust_cnt;
  semsignal(semwaiterid,waiter-'U');
  semsignal(mtxid,0);
  semwait(semcustomerid,cust_id-1);
  semwait(mtxid,0);
  int time=M[0];
  semsignal(mtxid,0);
  printf("%s\t Customer %d: Order placed to Waiter %c\n",time_str(time),cust_id,waiter);
  semwait(semcustomerid,cust_id-1);
  semwait(mtxid,0);
  time=M[0];
  semsignal(mtxid,0);
  printf("%s\t\t Customer %d gets food [Waiting time= %d]\n",time_str(time),cust_id,time-time_);
  usleep(30*TIME);
  semwait(mtxid,0);
  M[0]=time+30;
  M[1]++;
  semsignal(mtxid,0);
  printf("%s\t\t\t Customer %d finishes eating and leaves\n",time_str(time+30),cust_id);
  
  shmdt(M);
  exit(0);
}
int main(){
  int shmkey = ftok("/", 'A');
  check_error(shmkey, "ftok");
  int shmid = shmget(shmkey, 2000, 0666);
  check_error(shmid, "shmget");
  int* M = (int*)shmat(shmid, NULL, 0);
  check_error((int)(intptr_t)M, "shmat");

  int mtxkey = ftok("/", 'B');
  check_error(mtxkey, "ftok");
  int semcookkey = ftok("/", 'C');
  check_error(semcookkey, "ftok");
  int semwaiterkey = ftok("/", 'D');
  check_error(semwaiterkey, "ftok");
  int semcustomerkey = ftok("/", 'E');
  check_error(semcustomerkey, "ftok");

  int mtxid = semget(mtxkey, 1, 0666);
  check_error(mtxid, "semget");
  int semcookid = semget(semcookkey, 1, 0666);
  check_error(semcookid, "semget");
  int semwaiterid = semget(semwaiterkey, 5, 0666);
  check_error(semwaiterid, "semget");
  int semcustomerid = semget(semcustomerkey, 200, 0666);
  check_error(semcustomerid, "semget");

  FILE* fp = fopen(FILENAME, "r");
  if(fp == NULL){
    perror("fopen");
    fclose(fp);
    exit(1);
  }

  int prev_time=0;
  semwait(mtxid,0);
  int total_customer=0;
  while(1){
   
    int cust_id,time,cust_cnt;
    if(fscanf(fp, "%d %d %d", &cust_id, &time, &cust_cnt)!=3){
      semsignal(mtxid,0);
      break;
    }
    int diff=time-prev_time;
    if(diff>0){
      semsignal(mtxid,0);
      usleep(diff*TIME);
      semwait(mtxid,0);
    }
    if(cust_id ==-1){
      semsignal(mtxid,0);
      break;
    }
    total_customer++;
    pid_t pid;
    pid = fork();
    if(pid==0){
      cmain(cust_id, time, cust_cnt);
    }else{
      prev_time=time;
    }
  }
  for(int i=0;i<total_customer;i++){
    wait(NULL);
  }

  fclose(fp);
  shmctl(shmid, IPC_RMID, NULL);
  semctl(mtxid, 0, IPC_RMID);
  semctl(semcookid, 0, IPC_RMID);
  semctl(semwaiterid, 0, IPC_RMID);
  semctl(semcustomerid, 0, IPC_RMID);
  shmdt(M);
  return 0;
}
