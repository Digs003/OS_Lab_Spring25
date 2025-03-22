#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 4
#define TCOUNT 10
#define COUNT_LIMIT 12

int count=0;
pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;

void * watch_count(void *t){
  long my_id = (long) t;
  printf("Starting watch_count(): thread %ld\n", my_id);
  pthread_mutex_lock(&count_mutex);
  while(count<COUNT_LIMIT){
    printf("watch_count(): thread %ld Count=%d. Going into wait...\n",my_id, count);
    pthread_cond_wait(&count_threshold_cv, &count_mutex);
    printf("watch_count(): thread %ld Condition signal received. Count=%d\n", my_id, count);
  }
  printf("watch_count(): thread %ld Updating the value of count...\n", my_id);
  count += 125;
  printf("watch_count(): thread %ld count now = %d\n", my_id, count);
  printf("watch_count(): thread %ld Unlocking mutex\n", my_id);
  pthread_mutex_unlock(&count_mutex);
  pthread_exit(NULL);
}

void *inc_count(void *t){
  long my_id=(long) t;
  for(int i=0;i<TCOUNT;i++){
    pthread_mutex_lock(&count_mutex);
    count++;
    if(count == COUNT_LIMIT){
      printf("inc_count(): thread %ld count = %d. Threshold reached. Signaling condition...\n", my_id, count);
      pthread_cond_broadcast(&count_threshold_cv);
      printf("Just sent signal.\n");
    }
    printf("inc_count(): thread %ld count = %d. Unlocking mutex\n", my_id, count);
    pthread_mutex_unlock(&count_mutex);
    sleep(1);
  }
  pthread_exit(NULL);
}
     

int main(int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;
    long t1 = 1, t2 = 2, t3 = 3, t4=4;

    pthread_mutex_init(&count_mutex, NULL);
    pthread_cond_init(&count_threshold_cv, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&threads[0], &attr, watch_count, (void *) t1);
    pthread_create(&threads[1], &attr, inc_count, (void *) t2);
    pthread_create(&threads[2], &attr, inc_count, (void *) t3);
    pthread_create(&threads[3], &attr, watch_count, (void *) t4);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Main(): Waited and joined with %d threads. Final value of count = %d. Done.\n", NUM_THREADS, count);

    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&count_mutex);
    pthread_cond_destroy(&count_threshold_cv);
    pthread_exit(NULL);
}


