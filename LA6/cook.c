#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

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

void cmain(){
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

  int mtxid = semget(mtxkey, 1, 0666);
  check_error(mtxid, "semget");
  int semcookid = semget(semcookkey, 1, 0666);
  check_error(semcookid, "semget");
  int semwaiterid = semget(semwaiterkey, 5, 0666);
  check_error(semwaiterid, "semget");

  semwait(mtxid, 0);
  char cook=(getpid()==M[4])?'C':'D';
  int time=M[0];
  semsignal(mtxid,0);
  if(cook=='C'){
    printf("%s Cook %c is ready\n",time_str(time),cook);
  }else{
    printf("%s\t Cook %c is ready\n",time_str(time),cook);
  }
  while(1){
    semwait(semcookid,0);
    semwait(mtxid,0);
    int front=M[1100];
    int rear=M[1101];
    time=M[0];
    if(rear>front && time>240){
      if(cook=='C')printf("%s Cook %c: Leaving\n",time_str(time),cook);
      else printf("%s\t Cook %c: Leaving\n",time_str(time),cook);
      semsignal(mtxid,0);
      break;
    }
    char waiter=M[rear]+'U';
    int cust_id=M[rear+1];
    int cust_cnt=M[rear+2];
    M[1101]=rear+3;
    semsignal(mtxid,0);
    
    if(cook=='C'){
      printf("%s Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n",time_str(time),cook,waiter,cust_id,cust_cnt);
    }else{
      printf("%s\t Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n",time_str(time),cook,waiter,cust_id,cust_cnt);
    }
    int duration=5*cust_cnt*TIME;
    usleep(duration);
    semwait(mtxid,0);
    M[0]=time+5*cust_cnt;
    time=M[0];
    front=M[1100];
    rear=M[1101];
    M[200*(waiter-'U')+100]=cust_id;
    semsignal(mtxid,0);
    semsignal(semwaiterid,waiter-'U');
    
    if(cook=='C'){
      printf("%s Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n",time_str(time),cook,waiter,cust_id,cust_cnt);
    }else{
      printf("%s\t Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n",time_str(time),cook,waiter,cust_id,cust_cnt);
    }
    if(rear>front && time>240){
      if(cook=='C')printf("%s Cook %c: Leaving\n",time_str(time),cook);
      else printf("%s\t Cook %c: Leaving\n",time_str(time),cook);
      break;
    }
  }
  shmdt(M);
  exit(0);
}

int main(){
  key_t shmkey = ftok("/", 'A');
  check_error(shmkey, "ftok for shared memory failed");

  int shmid = shmget(shmkey, 2000 * sizeof(int), IPC_CREAT | 0666);
  check_error(shmid, "shmget failed");

  int* M = (int*)shmat(shmid, NULL, 0);
  if (M == (void*)-1) {
      perror("shmat failed");
      exit(EXIT_FAILURE);
  }

  for (int i = 0; i < 2000; i++) {
      M[i] = 0;
  }
  M[1] = 10;

  key_t mtxkey = ftok("/", 'B');
  check_error(mtxkey, "ftok for mutex semaphore failed");

  int mtxid = semget(mtxkey, 1, IPC_CREAT | 0666);
  check_error(mtxid, "semget for mutex failed");

  check_error(semctl(mtxid, 0, SETVAL, 1), "semctl SETVAL for mutex failed");

  key_t semcookkey = ftok("/", 'C');
  check_error(semcookkey, "ftok for cook semaphore failed");

  int semcookid = semget(semcookkey, 1, IPC_CREAT | 0666);
  check_error(semcookid, "semget for cook semaphore failed");

  check_error(semctl(semcookid, 0, SETVAL, 0), "semctl SETVAL for cook failed");

  key_t semwaiterkey = ftok("/", 'D');
  check_error(semwaiterkey, "ftok for waiter semaphore failed");

  int semwaiterid = semget(semwaiterkey, 5, IPC_CREAT | 0666);
  check_error(semwaiterid, "semget for waiter semaphore failed");

  for (int i = 0; i < 5; i++) {
      check_error(semctl(semwaiterid, i, SETVAL, 0), "semctl SETVAL for waiter failed");
  }

  key_t semcustomerkey = ftok("/", 'E');
  check_error(semcustomerkey, "ftok for customer semaphore failed");

  int semcustomerid = semget(semcustomerkey, 200, IPC_CREAT | 0666);
  check_error(semcustomerid, "semget for customer semaphore failed");

  for (int i = 0; i < 200; i++) {
      check_error(semctl(semcustomerid, i, SETVAL, 0), "semctl SETVAL for customer failed");
  }



  for(int i=0;i<2;i++){
    pid_t pid=fork();
    check_error(pid,"fork");
    if(pid==0){
      cmain();
    }else{
      semwait(mtxid,0);
      M[4+i]=pid;
      semsignal(mtxid,0);
    }
  }
  for(int i=0;i<2;i++){
    wait(NULL);
  }
  for(int i=0;i<5;i++){
    semsignal(semwaiterid,i);
  }

  shmdt(M);
  exit(0);
 }


