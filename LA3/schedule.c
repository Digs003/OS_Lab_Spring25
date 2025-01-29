#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>

#define MAX_SIZE 1024
#define FILENAME "proc.txt"
#define ARRIVED 0
#define IOCOMPLETE 1
#define TIMEOUT 2
#define BURSTCOMPLETE 3
#define FINISHED 4
int num_processes;
/*=================================Process Info=================================*/

typedef struct proc{
  int id;
  int arrival_time;
  int num_bursts;
  int* burst_times;
  int status;
  int current_burst;
  int burst_remaining;
  int total_time;
  int next_cpu_time;
}ProcInfo;

ProcInfo proc[MAX_SIZE];
/*=================================Queue Code Starts=================================*/
typedef struct queue{
  int data[MAX_SIZE];
  int front;         
  int rear;          
  int size;          
}Queue;

void init(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

int is_empty(Queue* q) {
    return q->size == 0;
}

int is_full(Queue* q) {
    return q->size == MAX_SIZE;
}

void enqueue(Queue* q, int value) {
    if (is_full(q)) {
        printf("Queue is full\n");
        return;
    }
    q->rear = (q->rear + 1) % MAX_SIZE; 
    q->data[q->rear] = value;
    q->size++;
}

int dequeue(Queue* q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    int value = q->data[q->front];
    q->front = (q->front + 1) % MAX_SIZE; 
    q->size--;
    return value;
}

int front(Queue* q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    return q->data[q->front];
}
/*=================================Queue Code Ends=================================*/

/*=================================MinHeap Code Starts=================================*/

typedef struct MinHeap {
    int* data;    
    int size;    
    int capacity;      
} MinHeap;

int parent(int i) {
    return (i - 1) / 2;
}

int left_child(int i) {
    return 2 * i + 1;
}


int right_child(int i) {
    return 2 * i + 2;
}


void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int compare(int a,int b){
    ProcInfo p1=proc[a];
    ProcInfo p2=proc[b];
    if(p1.next_cpu_time == p2.next_cpu_time){
        if(p1.status==p2.status){
            return p1.id-p2.id;
        }
        if(p1.status==ARRIVED || p1.status==IOCOMPLETE){
            return -1;
        }
        if(p2.status==ARRIVED || p2.status==IOCOMPLETE){
            return 1;
        }
        if(p1.status==TIMEOUT){
            return -1;
        }
        if(p2.status==TIMEOUT){
            return 1;
        }
    }
    return p1.next_cpu_time-p2.next_cpu_time;
}
void enqueue_heap(MinHeap* pq, int value) {
    if (pq->size == pq->capacity) {
        printf("Priority queue is full\n");
        return;
    }

    
    pq->data[pq->size] = value;
    int i = pq->size;
    pq->size++;

    // Heapify up
    while (i != 0 && compare(pq->data[i],pq->data[parent(i)])<0) {
        swap(&pq->data[parent(i)], &pq->data[i]);
        i = parent(i);
    }
}

void init_heap(MinHeap* pq,int count) {
    pq->size = 0;
    pq->data=(int*)malloc(sizeof(int)*MAX_SIZE);
    pq->capacity=count;
    for(int i=0;i<num_processes;i++){
        enqueue_heap(pq,i);
    }
}

void heapify_down(MinHeap* pq, int i) {
    int smallest = i;
    int left = left_child(i);
    int right = right_child(i);

    if (left < pq->size && compare(pq->data[left],pq->data[smallest])< 0) {
        smallest = left;
    }
    if (right < pq->size && compare(pq->data[right],pq->data[smallest])< 0) {
        smallest = right;
    }
    if (smallest != i) {
        swap(&pq->data[i], &pq->data[smallest]);
        heapify_down(pq, smallest);
    }
}

int dequeue_heap(MinHeap* pq) {
    if (pq->size == 0) {
        printf("Priority queue is empty\n");
        return INT_MAX;
    }

    int min_value = pq->data[0];
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;

    // Heapify down
    heapify_down(pq, 0);

    return min_value;
}

int get_min(MinHeap* pq) {
    if (pq->size == 0) {
        printf("Priority queue is empty\n");
        return INT_MAX;
    }
    return pq->data[0];
}

/*=================================MinHeap Code Ends=================================*/

/*=================================Reading from File=================================*/

void proc_init(){
  FILE* fp=fopen(FILENAME,"r");
  if(fp==NULL){
    printf("Error in opening input file\n");
    exit(1);
  }

  
  fscanf(fp,"%d",&num_processes);
  for(int i=0;i<num_processes;i++){
    int id;
    fscanf(fp,"%d",&id);
    proc[i].id=id;
    int arrival_time;
    fscanf(fp,"%d",&arrival_time);
    proc[i].arrival_time=arrival_time;
    proc[i].next_cpu_time=arrival_time;
    proc[i].num_bursts=0;
    proc[i].burst_times=(int*)malloc(sizeof(int)*MAX_SIZE);
    int time;
    while(1){
      fscanf(fp,"%d",&time);
      if(time==-1){
        break;
      }
      proc[i].burst_times[proc[i].num_bursts]=time;
      proc[i].total_time+=time;
      proc[i].num_bursts++;
    }
    proc[i].status=ARRIVED;
    proc[i].current_burst=0;
    proc[i].burst_remaining=proc[i].burst_times[0];
 }
 fclose(fp);
}

/*=================================Scheduling Implementation=================================*/

void print_process_stats(int i,int turnaround_time,int wait_time,int time){
    ProcInfo p=proc[i];
    int turnaround_percentage=(int)((turnaround_time*100.0)/p.total_time+0.5);
    printf("%d\t\t: Process %d exits. Turnaround time= %d ( %d%% ), Wait time=%d\n",time,p.id,turnaround_time,turnaround_percentage,wait_time);
}


void round_robin(int q){
  proc_init();
  MinHeap* event_queue;
  event_queue=(MinHeap*)malloc(sizeof(MinHeap));
  init_heap(event_queue,num_processes);
  Queue* ready_queue;
  ready_queue=(Queue*)malloc(sizeof(Queue));
  init(ready_queue);
  int time=0;
  int cpu_idle_time=0;
  int running_process=-1;
  int cpu_idle_start=-1;
  double total_waiting_time=0;
  #ifdef VERBOSE
  printf("%d\t\t: Starting\n",time);
  #endif
  while(event_queue->size>0){
    int curr_process_idx=dequeue_heap(event_queue);
    ProcInfo* curr_process= &proc[curr_process_idx];
    //printf("DEBUG:Process %d\n",curr_process->id);
    if(running_process==-1 && cpu_idle_start!=-1){
        cpu_idle_time+=curr_process->next_cpu_time-cpu_idle_start;
        cpu_idle_start=curr_process->next_cpu_time;
    }
    time=curr_process->next_cpu_time;

    if(curr_process->status==ARRIVED){
        #ifdef VERBOSE
        printf("%d\t\t: Process %d joins ready queue upon arrival\n",time,curr_process->id);
        #endif
        // printf("DEBUG:Process %d next cpu time=%d\n",curr_process->id,curr_process->next_cpu_time);
        // printf("DEBUG:Running process=%d\n",running_process);
        enqueue(ready_queue,curr_process_idx);
        
        //printf("DEBUG:Ready queue size=%d\n",ready_queue->size);
    }
    else if(curr_process->status==IOCOMPLETE){
        
        #ifdef VERBOSE
        printf("%d\t\t: Process %d joins ready queue after IO completion\n",time,curr_process->id);
        #endif
        curr_process->burst_remaining=curr_process->burst_times[curr_process->current_burst];
        enqueue(ready_queue,curr_process_idx);
    }
    else if(curr_process->status==TIMEOUT){
        #ifdef VERBOSE
        printf("%d\t\t: Process %d joins ready queue after timeout\n",time,curr_process->id);
        #endif
        running_process=-1;
        enqueue(ready_queue,curr_process_idx);
    }
    else if(curr_process->status==BURSTCOMPLETE){
        running_process=-1;
        curr_process->current_burst+=2;
        if(curr_process->current_burst>=curr_process->num_bursts){
            curr_process->status=FINISHED;
            int turnaround_time=time-curr_process->arrival_time;
            int wait_time=turnaround_time-curr_process->total_time;
            total_waiting_time+=wait_time;
            print_process_stats(curr_process_idx,turnaround_time,wait_time,time);
        }else{  
            curr_process->status=IOCOMPLETE;
            curr_process->next_cpu_time=time+curr_process->burst_times[curr_process->current_burst-1];
            enqueue_heap(event_queue,curr_process_idx);
        }
    }
    if(running_process==-1){
        
        if(!is_empty(ready_queue)){
            
            running_process=dequeue(ready_queue);
            //printf("DEBUG:Running process=%d\n",running_process);
            curr_process=&proc[running_process];
            int run_time=(curr_process->burst_remaining<q || q==INT_MAX)?curr_process->burst_remaining:q;
            
            if(cpu_idle_start!=-1){
                cpu_idle_time+=time-cpu_idle_start;
                cpu_idle_start=-1;
            }
            #ifdef VERBOSE
            printf("%d\t\t: Process %d is scheduled to run for time %d\n",time,curr_process->id,run_time);
            #endif
            curr_process->next_cpu_time=time+run_time;
            curr_process->status=(run_time==curr_process->burst_remaining)?BURSTCOMPLETE:TIMEOUT;
            if(curr_process->status==TIMEOUT){
                curr_process->burst_remaining-=q;
            }
            enqueue_heap(event_queue,running_process);
        }else{
            
            if(running_process==-1 && cpu_idle_start==-1){
                cpu_idle_start=time;
            }
            if(time>0){
                #ifdef VERBOSE
                printf("%d\t\t: CPU goes idle\n",time);
                #endif
            }
        }
    }
  }
  if(cpu_idle_start!=-1){
    cpu_idle_time+=time-cpu_idle_start;
  }
  double avg_waiting_time=total_waiting_time/num_processes;
  double cpu_utilization=(time-cpu_idle_time)*100.0/time;
  printf("\nAverage waiting time=%.2f\n",avg_waiting_time);
  printf("Total turnaround time=%d\n",time);
  printf("CPU idle time=%d\n",cpu_idle_time);
  printf("CPU utilization=%.2f%%\n",cpu_utilization);
  memset(proc,0,sizeof(proc));
}


int main(){
  
  printf("\n**** FCFS Scheduling ****\n");
  round_robin(INT_MAX);
  printf("\n**** RR Scheduling with q=10 ****\n");
  round_robin(10);  
  printf("\n**** RR Scheduling with q=5 ****\n");
  round_robin(5);
  exit(0);
}


