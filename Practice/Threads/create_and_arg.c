#include<stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5

char* messages[NUM_THREADS];

struct thread_data{
  int thread_id;
  int sum;
  char* message;
};

struct thread_data thread_data_array[NUM_THREADS];

void *PrintHello(void* threadarg){
  sleep(1);
  struct thread_data* my_data;
  my_data=(struct thread_data*)threadarg;
  int taskid, sum;
  char* hello_msg;
  taskid=my_data->thread_id;
  sum=my_data->sum;
  hello_msg=my_data->message;
  printf("Thread %d: %s Sum=%d\n",taskid,hello_msg,sum);
  pthread_exit(NULL);
}


int main(){
  pthread_t threads[NUM_THREADS];
  int rc,t,sum;
  messages[0] = "English: Hello World!";
  messages[1] = "French: Bonjour, le monde!";
  messages[2] = "Spanish: Hola al mundo!";
  messages[3] = "Klingon: Nuq neH!";
  messages[4] = "German: Guten Tag, Welt!";

  for(t=0;t<NUM_THREADS;t++){
    sum=sum+t;
    thread_data_array[t].thread_id = t;
    thread_data_array[t].sum = sum;
    thread_data_array[t].message = messages[t];
    printf("Creating thread %d\n",t);
    rc = pthread_create(&threads[t],NULL,PrintHello,(void*)&thread_data_array[t]);
    if(rc){
      printf("ERROR; return code from pthread_create() is %d\n",rc);
      exit(-1);
    }
  }
  pthread_exit(NULL);
}
  
