#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct{
  double* a;
  double* b;
  double sum;
  int veclen;
} DOTDATA;

#define NUMTHRDS 4
#define VECLEN 100000

DOTDATA dotstr;
pthread_t callThd[NUMTHRDS];
pthread_mutex_t mutexsum;

void* dotprod(void* arg){
  int len=dotstr.veclen;
  long offset=(long)arg;
  int start=offset*len;
  int end=start+len;
  double *x=dotstr.a;
  double *y=dotstr.b;
  double mysum=0;
  for(int i=start; i<end; i++){
    mysum+=x[i]*y[i];
  }
  pthread_mutex_lock(&mutexsum);
  dotstr.sum+=mysum;
  printf("Thread %ld did %d to %d: mysum=%f global sum=%f\n", offset, start, end, mysum, dotstr.sum);
  pthread_mutex_unlock(&mutexsum);
  pthread_exit((void*)0);
}

int main(){
  double *a,*b;
  a=(double*)malloc(NUMTHRDS*VECLEN*sizeof(double));
  b=(double*)malloc(NUMTHRDS*VECLEN*sizeof(double));
  for(int i=0; i<VECLEN*NUMTHRDS; i++){
    a[i]=1;
    b[i]=a[i];
  }
  dotstr.veclen=VECLEN;
  dotstr.a=a;
  dotstr.b=b;
  dotstr.sum=0;

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_mutex_init(&mutexsum, NULL);

  for(int i=0;i<NUMTHRDS;i++){
    pthread_create(&callThd[i], &attr, dotprod, (void*)i);
  }
  pthread_attr_destroy(&attr);

  for(int i=0;i<NUMTHRDS;i++){
    pthread_join(callThd[i], NULL);
  }
  printf("Sum=%f\n", dotstr.sum);
  free(a);
  free(b);
  pthread_mutex_destroy(&mutexsum);
  pthread_exit(NULL);
}

