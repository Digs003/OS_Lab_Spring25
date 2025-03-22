#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

typedef struct{
  int value;
  pthread_mutex_t mtx;
  pthread_cond_t cv;
}semaphore;

semaphore rider;
pthread_mutex_t bmtx;
pthread_barrier_t EOS;
int *BA,*BC,*BT;
int m,n,visitor_count;
pthread_barrier_t *BB;

void P(semaphore *s){
  pthread_mutex_lock(&s->mtx);
  while(s->value <=0){
    pthread_cond_wait(&s->cv, &s->mtx);
  }
  s->value--;
  pthread_mutex_unlock(&s->mtx);
}

void V(semaphore *s){
  pthread_mutex_lock(&s->mtx);
  s->value++;
  pthread_cond_signal(&s->cv);
  pthread_mutex_unlock(&s->mtx);
}

void *boat_thread(void* t){
  long boat_id=(long)t;
  printf("Boat %6ld\tready\n",boat_id);
  pthread_mutex_lock(&bmtx);
  BA[boat_id-1]=1;
  BC[boat_id-1]=-1;
  pthread_mutex_unlock(&bmtx);
  pthread_barrier_init(&BB[boat_id-1],NULL,2);
  
  while(1){
    V(&rider);
    pthread_barrier_wait(&BB[boat_id-1]);
    printf("Boat %6ld\tStart of ride for visitor %d\n",boat_id,BC[boat_id-1]);
    pthread_mutex_lock(&bmtx);
    BA[boat_id-1]=0;
    int time=BT[boat_id-1];
    pthread_mutex_unlock(&bmtx);
    usleep(time*100000);
    //usleep(1000);
    printf("Boat %6ld\tEnd of ride for visitor %d (ride time = %d)\n",boat_id,BC[boat_id-1],time);
    pthread_mutex_lock(&bmtx);
    BA[boat_id-1]=1;
    BC[boat_id-1]=-1;
    visitor_count--;
    pthread_mutex_unlock(&bmtx);
    if(visitor_count==0){
      pthread_barrier_wait(&EOS);
      break;
    }
  }
  return NULL;
}

void *visitor_thread(void* t){
  long visitor_id=(long)t;
  int sightseeing_time=rand()%91+30;
  int ride_time=rand()%46+15;
  printf("Visitor %3ld\tStarts sightseeing for %3d minutes\n",visitor_id,sightseeing_time);
  usleep(sightseeing_time*100000);
  printf("Visitor %3ld\tReady to ride a boat (ride time = %d)\n",visitor_id,ride_time);

  P(&rider);
  int boat_idx;
  //No Busy waiting needed because P(&rider) ensures that this part executes
  //only if atleast one boat is active
  pthread_mutex_lock(&bmtx);
  for(int i=0;i<m;i++){
    if(BA[i]==1 && BC[i]==-1){
      BC[i]=visitor_id;
      BT[i]=ride_time;
      boat_idx=i;
      break;
    }
  }
  pthread_mutex_unlock(&bmtx);
  
  pthread_barrier_wait(&BB[boat_idx]);
  printf("Visitor %3ld\tFinds boat %3d\n",visitor_id,boat_idx+1);
  usleep(ride_time*100000);
  printf("Visitor %3ld\tLeaving\n",visitor_id);
  return NULL;
}








int main(int argc,char*argv[]){
  if(argc!=3){
    printf("Usage: %s <m> <n>\n",argv[0]);
    return 1;
  }
  srand(time(NULL));
  m = atoi(argv[1]);
  n = atoi(argv[2]);
  visitor_count=n;

  //No need for boat semaphore to implement visitor thread with no busy waiting
  //Barrier Wait Suffices
  //boat =(semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
  rider=(semaphore){0,PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER};
  pthread_mutex_init(&bmtx,NULL);
  pthread_barrier_init(&EOS,NULL,2);
  pthread_t boats[m],visitors[n];
  BA=(int*)malloc(m*sizeof(int));
  BC=(int*)malloc(m*sizeof(int));
  BT=(int*)malloc(m*sizeof(int));
  BB=(pthread_barrier_t*)malloc(m*sizeof(pthread_barrier_t));

  for(long i=0;i<m;i++){
    pthread_create(&boats[i],NULL,boat_thread,(void*)(i+1));
  }
  for(long i=0;i<n;i++){
    pthread_create(&visitors[i],NULL,visitor_thread,(void*)(i+1));
  }
  pthread_barrier_wait(&EOS);

  free(BA);
  free(BC);
  free(BT);
  free(BB);

  pthread_barrier_destroy(&EOS);
  pthread_mutex_destroy(&bmtx);
  pthread_mutex_destroy(&rider.mtx);
  pthread_cond_destroy(&rider.cv);

  return 0;
}
  
