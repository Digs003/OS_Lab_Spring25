#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>

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

void check_error(int result, const char* msg) {
    if (result == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
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

void place_order(int time,char waiter,int cust_id,int cust_cnt){
  if(waiter=='U'){
    printf("%s Waiter %c: Placing order for Customer %d (count=%d)\n",time_str(time),waiter,cust_id,cust_cnt);
  }else if(waiter=='V'){
    printf("%s\t Waiter %c: Placing order for Customer %d (count=%d)\n",time_str(time),waiter,cust_id,cust_cnt);
  }else if(waiter=='W'){
    printf("%s\t\t Waiter %c: Placing order for Customer %d (count=%d)\n",time_str(time),waiter,cust_id,cust_cnt);
  }else if(waiter=='X'){
    printf("%s\t\t\t Waiter %c: Placing order for Customer %d (count=%d)\n",time_str(time),waiter,cust_id,cust_cnt);
  }else if(waiter=='Y'){
    printf("%s\t\t\t\t Waiter %c: Placing order for Customer %d (count=%d)\n",time_str(time),waiter,cust_id,cust_cnt);
  }
}

void serve_customer(int time,char waiter,int cust_id){
  if(waiter=='U'){
    printf("%s Waiter %c: Serving food to Customer %d\n",time_str(time),waiter,cust_id);
  }else if(waiter=='V'){
    printf("%s\t Waiter %c: Serving food to Customer %d\n",time_str(time),waiter,cust_id);
  }else if(waiter=='W'){
    printf("%s\t\t Waiter %c: Serving food to Customer %d\n",time_str(time),waiter,cust_id);
  }else if(waiter=='X'){
    printf("%s\t\t\t Waiter %c: Serving food to Customer %d\n",time_str(time),waiter,cust_id);
  }else if(waiter=='Y'){
    printf("%s\t\t\t\t Waiter %c: Serving food to Customer %d\n",time_str(time),waiter,cust_id);
  }
}

void waiter_leave(int time,char waiter){
  if(waiter=='U'){
    printf("%s Waiter %c leaving (no more customer to serve)\n",time_str(time),waiter);
  }else if(waiter=='V'){
    printf("%s\t Waiter %c leaving (no more customer to serve)\n",time_str(time),waiter);
  }else if(waiter=='W'){
    printf("%s\t\t Waiter %c leaving (no more customer to serve)\n",time_str(time),waiter);
  }else if(waiter=='X'){
    printf("%s\t\t\t Waiter %c leaving (no more customer to serve)\n",time_str(time),waiter);
  }else if(waiter=='Y'){
    printf("%s\t\t\t\t Waiter %c leaving (no more customer to serve)\n",time_str(time),waiter);
  }
}

void wmain(){
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

    semwait(mtxid, 0);
    pid_t pid = getpid();
    char waiter = 'U';
    if (pid == M[6]) {
        waiter = 'U';
    } else if (pid == M[7]) {
        waiter = 'V';
    } else if (pid == M[8]) {
        waiter = 'W';
    } else if (pid == M[9]) {
        waiter = 'X';
    } else if (pid == M[10]) {
        waiter = 'Y';
    }
    int time = M[0];
    semsignal(mtxid, 0);

    if (waiter == 'U') {
        printf("%s Waiter %c is ready\n", time_str(time), waiter);
    } else if (waiter == 'V') {
        printf("%s\t Waiter %c is ready\n", time_str(time), waiter);
    } else if (waiter == 'W') {
        printf("%s\t\t Waiter %c is ready\n", time_str(time), waiter);
    } else if (waiter == 'X') {
        printf("%s\t\t\t Waiter %c is ready\n", time_str(time), waiter);
    } else if (waiter == 'Y') {
        printf("%s\t\t\t\t Waiter %c is ready\n", time_str(time), waiter);
    }
    int pending = 0;
    while (1) {
        semwait(semwaiterid, waiter - 'U');
        semwait(mtxid, 0);
        time = M[0];
        int rear = M[200 * (waiter - 'U') + 103];
        int front = M[200 * (waiter - 'U') + 102];

        if (M[200 * (waiter - 'U') + 100] != 0) {
            int cust_id = M[200 * (waiter - 'U') + 100];
            serve_customer(time, waiter, cust_id);
            pending--;
            M[200 * (waiter - 'U') + 100] = 0;
            semsignal(semcustomerid, cust_id - 1);
            if (rear > front && time > 240 && pending == 0) {
                waiter_leave(time, waiter);
                semsignal(semcookid, 0);
                semsignal(mtxid, 0);
                break;
            }
        }
        if ((front == 0 || rear > front) && time > 240 && pending == 0) {
            waiter_leave(time, waiter);
            semsignal(semcookid, 0);
            semsignal(mtxid, 0);
            break;
        }

        if (rear == 0) {
            semsignal(mtxid, 0);
            continue;
        }
        int cust_id = M[rear];
        if (cust_id == 0) {
            semsignal(mtxid, 0);
            continue;
        }
        int cust_cnt = M[rear + 1];
        M[200 * (waiter - 'U') + 103] += 2;
        time = M[0];
        semsignal(mtxid, 0);
        usleep(TIME);
        semsignal(semcustomerid, cust_id - 1);
        semwait(mtxid, 0);
        M[0] = time + 1;
        time = M[0];

        //write order to cook's queue
        front = M[1100];
        if (front == 0) {
            front = 1102;
            M[1100] = 1102;
            M[1101] = 1102;
        } else {
            M[1100] = front + 3;
            front = M[1100];
        }
        M[front] = waiter - 'U';
        M[front + 1] = cust_id;
        M[front + 2] = cust_cnt;
        semsignal(mtxid, 0);
        place_order(time, waiter, cust_id, cust_cnt);
        pending++;
        semsignal(semcookid, 0);
    }
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
    int mtxid = semget(mtxkey, 1, 0666);
    check_error(mtxid, "semget");

    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        check_error(pid, "fork");
        if (pid == 0) {
            wmain();
        } else {
            semwait(mtxid, 0);
            M[6 + i] = pid;
            semsignal(mtxid, 0);
        }
    }
    for (int i = 0; i < 5; i++) {
        wait(NULL);
    }

    shmdt(M);
    return 0;
}
