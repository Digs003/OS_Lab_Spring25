#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>


#define MAX_PROCESSES 500
#define MAX_SEARCHES 100
#define PAGES_PER_PROCESS 2048
#define TOTAL_FRAMES 12288
#define ESSENTIAL_PAGES 10
#define NFFMIN 1000

#define SET_VALID(x) ((x) |= (1<<15))
#define CLEAR_VALID(x) ((x) &= ~(1<<15))
#define IS_VALID(x) ((x) & (1<<15))

#define SET_REFERENCE(x) ((x) |=(1<<14))
#define CLEAR_REFERENCE(x) ((x) &= ~(1<<14))
#define IS_REFERENCE(x) ((x) & (1<<14))

typedef struct {
    int frame;
    int pid;
    int page_number;
} Frame;


 
typedef struct{
    unsigned short entry;
    unsigned short history;
}PAGE_ENTRY;

typedef struct{
  int size; //size of array A
  int *search_indices;
  PAGE_ENTRY *page_table;
  int searches_done;
  bool is_active;
} Process;

typedef struct {
    int front;
    int rear;
    Frame items[TOTAL_FRAMES];
    int size;
} FrameQueue;

typedef struct {
    int front;
    int rear;
    int items[MAX_PROCESSES];
    int size;
} Queue;

void queue_init(Queue* q) {
    q->front = q->rear = -1;
    q->size = 0;
}



bool queue_is_empty(Queue* q) {
    return q->size == 0;
}

void queue_enqueue(Queue* q, int item) {
    if (q->size == MAX_PROCESSES) {
        fprintf(stderr, "Queue overflow\n");
        exit(1);
    }
    
    if (q->rear == -1) {
        q->front = q->rear = 0;
    } else {
        q->rear = (q->rear + 1) % MAX_PROCESSES;
    }
    
    q->items[q->rear] = item;
    q->size++;
}

int queue_dequeue(Queue* q) {
    if (queue_is_empty(q)) {
        fprintf(stderr, "Queue underflow\n");
        exit(1);
    }
    
    int item = q->items[q->front];
    
    if (q->size == 1) {
        q->front = q->rear = -1;
    } else {
        q->front = (q->front + 1) % MAX_PROCESSES;
    }
    
    q->size--;
    return item;
}




bool frame_queue_is_empty(FrameQueue* fq) {
    return fq->size == 0;
}

void frame_queue_enqueue(FrameQueue* fq, Frame frame) {
    if (fq->size == TOTAL_FRAMES) {
        fprintf(stderr, "Frame queue overflow\n");
        exit(1);
    }
    
    if (fq->rear == -1) {
        fq->front = fq->rear = 0;
    } else {
        fq->rear = (fq->rear + 1) % TOTAL_FRAMES;
    }
    
    fq->items[fq->rear] = frame;
    fq->size++;
}


Frame frame_queue_dequeue(FrameQueue* fq) {
    if (frame_queue_is_empty(fq)) {
        fprintf(stderr, "No free frames available\n");
        exit(1);  // Handle error
    }
    
    Frame frame = fq->items[fq->front];

    if (fq->size == 1) {
        fq->front = fq->rear = -1;
    } else {
        fq->front = (fq->front + 1) % TOTAL_FRAMES;
    }
    
    fq->size--;
    return frame;
}

void frame_queue_init(FrameQueue* fq) {
    fq->front = fq->rear = -1;
    fq->size = 0;

    // Initialize the queue with available frames
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        Frame frame = { .frame = i, .pid = -1, .page_number = -1 };
        frame_queue_enqueue(fq, frame);
    }
}


int n,m,NFF;
Process* processes;
 
FrameQueue FFLIST;
Queue ready_queue;
Frame temp_queue[TOTAL_FRAMES];
int page_faults[MAX_PROCESSES];
int page_accesses[MAX_PROCESSES];
int page_replacements[MAX_PROCESSES];
int attempts[MAX_PROCESSES][4];


int LRU_Replacement(Process* p){
    int min_history = 0xFFFF;
    int min_index = -1;
    for(int i=10;i<PAGES_PER_PROCESS;i++){
        if(IS_VALID(p->page_table[i].entry)){
            if(p->page_table[i].history < min_history){
                min_history = p->page_table[i].history;
                min_index = i;
            }
        }
    }
    if(min_index == -1){
        for(int i=10;i<PAGES_PER_PROCESS;i++){
            if(IS_VALID(p->page_table[i].entry)){
                if(p->page_table[i].history == 0xFFFF){
                    min_index = i;
                    break;
                }
            }
        }
    }
    return min_index;
}



void find_free_frame(Process* p,int page_index,int process_number,int victim){
    int idx=0;
    while(!frame_queue_is_empty(&FFLIST)){
       temp_queue[idx] = frame_queue_dequeue(&FFLIST);
       idx++;
    }

    //Attempt 1
    for(int i=0;i<NFF;i++){
        if(temp_queue[i].pid == process_number && temp_queue[i].page_number == page_index){
            Frame *frame = &temp_queue[i];
            #ifdef VERBOSE 
                printf("\t\tAttempt 1: Page found in free frame %d\n",frame->frame);
            #endif
            int victim_frame = p->page_table[victim].entry & 0x3FFF;
            Frame freed;
            freed.frame = victim_frame;
            freed.pid = process_number;
            freed.page_number = victim;
            temp_queue[i] = freed;
            p->page_table[page_index].entry = frame->frame;
            SET_VALID(p->page_table[page_index].entry);
            attempts[process_number][0]++;
            return;
        }
    }
    //Attempt 2
    for(int i=0;i<NFF;i++){
        if(temp_queue[i].pid ==-1){
            
            Frame *frame = &temp_queue[i];
            #ifdef VERBOSE 
                printf("\t\tAttempt 2: Free frame %d owned by no process found\n",frame->frame);
            #endif
            int victim_frame = p->page_table[victim].entry & 0x3FFF;
            Frame freed;
            freed.frame = victim_frame;
            freed.pid = process_number;
            freed.page_number = victim;
            temp_queue[i] = freed;
            p->page_table[page_index].entry = frame->frame;
            SET_VALID(p->page_table[page_index].entry);
            attempts[process_number][1]++;
            return;
        }
    }
    //Attempt 3
    for(int i=0;i<NFF;i++){
        if( temp_queue[i].pid == process_number){
            int prev_page = temp_queue[i].page_number;
            Frame *frame = &temp_queue[i];
            #ifdef VERBOSE 
                printf("\t\tAttempt 3: Own page %d found in free frame %d\n",prev_page,frame->frame);
            #endif
            int victim_frame = p->page_table[victim].entry & 0x3FFF;
            Frame freed;
            freed.frame = victim_frame;
            freed.pid = process_number;
            freed.page_number = victim;
            temp_queue[i] = freed;
            p->page_table[page_index].entry = frame->frame;
            SET_VALID(p->page_table[page_index].entry);
            attempts[process_number][2]++;
            return;
        }
    }
    //Attempt 4
    int i = rand()%NFF;
    Frame *frame = &temp_queue[i];
    int owner = frame->pid;
    #ifdef VERBOSE 
        printf("\t\tAttempt 4: Free frame %d owned by Process %d chosen\n",frame->frame,owner);
    #endif
    int victim_frame = p->page_table[victim].entry & 0x3FFF;
    Frame freed;
    freed.frame = victim_frame;
    freed.pid = process_number;
    freed.page_number = victim;
    temp_queue[i] = freed;
    p->page_table[page_index].entry = frame->frame;
    SET_VALID(p->page_table[page_index].entry);
    attempts[process_number][3]++;
    return;
        
    
}



void load_page(Process* p, int page_index,int process_number) {
    page_faults[process_number]++;

    if(NFF>NFFMIN){
        Frame frame;
        frame = frame_queue_dequeue(&FFLIST);
        NFF--;
        frame.pid = process_number;
        frame.page_number = page_index;
        p->page_table[page_index].entry = frame.frame;
        SET_VALID(p->page_table[page_index].entry);
        #ifdef VERBOSE
            printf("\tFault on page %d: Free frame %d found\n",page_index,frame.frame);
        #endif

    }else{
        page_replacements[process_number]++;
        int victim_page = LRU_Replacement(p);
        int frame = p->page_table[victim_page].entry & 0x3FFF;
        int history = p->page_table[victim_page].history;

        #ifdef VERBOSE
            printf("\tFault on page %d: To replace Page %d at Frame %d [history = %d]\n",page_index,victim_page,frame,history);
        #endif

        find_free_frame(p,page_index,process_number,victim_page);
        for(int i=0;i<NFF;i++){
            frame_queue_enqueue(&FFLIST,temp_queue[i]);
        }
        // Update the page table entry of victim page
        p->page_table[victim_page].entry = 0;
        p->page_table[victim_page].history = 0;
    }
}

void remove_process(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;

    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (IS_VALID(p->page_table[i].entry)) {
            int frame = p->page_table[i].entry & 0x3FFF;
            Frame freeframe ;
            freeframe.frame = frame;
            freeframe.pid = -1;
            freeframe.page_number = -1;
            frame_queue_enqueue(&FFLIST, freeframe);
            NFF++;
            p->page_table[i].entry = 0;
            p->page_table[i].history = 0;
        }
    }

}

bool binary_search(int process_number) {
    Process *p = &processes[process_number];
    int search_key = p->search_indices[p->searches_done];
    int L = 0, R = p->size - 1;

    while (L < R) {
        int M = (L + R) / 2;
        int page_index = 10 + M / 1024;
        
        if (!IS_VALID(p->page_table[page_index].entry)) {
            load_page(p, page_index,process_number);
            p->page_table[page_index].history = 0xFFFF;
        }

        page_accesses[process_number]++;
        SET_REFERENCE(p->page_table[page_index].entry);

        if (search_key <= M) {
            R = M;
        } else {
            L = M + 1;
        }

        // Update the history of all pages
        
    }
    for(int i=10;i<PAGES_PER_PROCESS;i++){
        if(IS_VALID(p->page_table[i].entry)){
             p->page_table[i].history = (unsigned short)(p->page_table[i].history >> 1);
            if(IS_REFERENCE(p->page_table[i].entry)){
                p->page_table[i].history |= (1<<15);
                CLEAR_REFERENCE(p->page_table[i].entry);
            }
        }
    }
    p->searches_done++;
    return true;
}



void run_simulation() {

    
    while (1) {
        bool finished = true;
        for (int i = 0; i < n; i++) {
            if (processes[i].searches_done < m) {
                finished = false;
                break;
            }
        }
        if (finished) break;

        int curr_process = queue_dequeue(&ready_queue);
        Process* p = &processes[curr_process];

        if (p->is_active && p->searches_done < m) {
            #ifdef VERBOSE
                printf("+++Process %d: Search %d\n", curr_process,p->searches_done + 1);
            #endif
            
            if (binary_search(curr_process)) {
                if (p->searches_done < m) {
                    queue_enqueue(&ready_queue, curr_process);
                } else {
                    remove_process(curr_process);
                }
            }
        }
    }
}



void read_input() {
    FILE* fp = fopen("sample/search.txt", "r");
    if (fp == NULL) {
        printf("Error opening input file\n");
        exit(1);
    }
    
    fscanf(fp, "%d %d", &n, &m);
    
    processes = (Process*)malloc(n * sizeof(Process));
    
    for (int i = 0; i < n; i++) {
        Process* p = &processes[i];
        p->is_active = true;
        p->searches_done = 0;
        p->page_table = calloc(PAGES_PER_PROCESS, sizeof(PAGE_ENTRY));
        
        fscanf(fp, "%d", &p->size);
        p->search_indices = (int*)malloc(m * sizeof(int));
        
        for (int j = 0; j < m; j++) {
            fscanf(fp, "%d", &p->search_indices[j]);
        }
        
        for (int j = 0; j < ESSENTIAL_PAGES; j++) {
            Frame frame;
            if(NFF>NFFMIN){
                frame= frame_queue_dequeue(&FFLIST);
                NFF--;
            }else{
                printf("ERROR: No free frames available\n");
                exit(1);
            }
            frame.pid = i;
            frame.page_number = j;
            p->page_table[j].entry = frame.frame;
            p->page_table[j].history = 0xFFFF;
            SET_VALID(p->page_table[j].entry);
            SET_REFERENCE(p->page_table[j].entry);
        }
        //printf("Ready Queue size = %d\n",ready_queue.size);
        queue_enqueue(&ready_queue, i);
    }
    
    fclose(fp);
}

void init_fflist(){
    NFF = TOTAL_FRAMES;
    frame_queue_init(&FFLIST);
}

void print_results(){
    printf("+++ Page access summary\n");

    int total_accesses = 0, total_faults = 0, total_replacements = 0, total_attempts[4] = {0};
    printf("PID     Accesses        Faults         Replacements                        Attempts\n");
    for (int i = 0; i < n; i++) {
        total_accesses += page_accesses[i];
        total_faults += page_faults[i];
        total_replacements += page_replacements[i];
        for (int j = 0; j < 4; j++) {
            total_attempts[j] += attempts[i][j];
        }
        double fault_percent = (double) page_faults[i] / page_accesses[i] * 100;
        double replacement_percent = (double) page_replacements[i] / page_accesses[i] * 100;
        double attempt_percent[4];
        for (int j = 0; j < 4; j++) {
            attempt_percent[j] = (double) attempts[i][j] / page_replacements[i] * 100;
        }
        printf("%4d  %10d  %7d   (%5.2f%%)  %7d   (%5.2f%%)   %3d + %3d + %3d + %3d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
               i, page_accesses[i], page_faults[i], fault_percent, page_replacements[i], replacement_percent,
               attempts[i][0], attempts[i][1], attempts[i][2], attempts[i][3],
               attempt_percent[0], attempt_percent[1], attempt_percent[2], attempt_percent[3]);
    }
    double total_fault_percent = (double) total_faults / total_accesses * 100;
    double total_replacement_percent = (double) total_replacements / total_accesses * 100;
    double total_attempt_percent[4];
    for (int j = 0; j < 4; j++) {
        total_attempt_percent[j] = (double) total_attempts[j] / total_replacements * 100;
    }
    printf("\nTotal     %8d  %7d (%5.2f%%)  %9d (%5.2f%%)   %3d + %3d + %3d + %d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
           total_accesses, total_faults, total_fault_percent, total_replacements, total_replacement_percent,
           total_attempts[0], total_attempts[1], total_attempts[2], total_attempts[3],
           total_attempt_percent[0], total_attempt_percent[1], total_attempt_percent[2], total_attempt_percent[3]);
}

int main() {
    srand(time(NULL));
    init_fflist();
    queue_init(&ready_queue);
    
    
    for(int i=0;i<MAX_PROCESSES;i++){
        page_faults[i] = 0;
        page_accesses[i] = 0;
        page_replacements[i] = 0;
        for(int j=0;j<4;j++){
            attempts[i][j] = 0;
        }
    }

    read_input();
    run_simulation();
    print_results();

    for (int i = 0; i < n; i++) {
        free(processes[i].search_indices);
        free(processes[i].page_table);
    }
    free(processes);

    return 0;
}