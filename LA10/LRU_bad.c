#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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
    int is_free;
    int pid;
    int page_number;
} Frame;

typedef struct {
    int frames[TOTAL_FRAMES];
    int count;
} FrameList;



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

int n,m,NFF;
Process* processes;
Frame FFLIST[TOTAL_FRAMES];
FrameList free_frames;           
FrameList process_free_frames[MAX_PROCESSES]; 
Queue ready_queue;
int page_faults[MAX_PROCESSES];
int page_accesses[MAX_PROCESSES];
int page_replacements[MAX_PROCESSES];
int attempts[MAX_PROCESSES][4];

void add_to_free_list(int frame_index) {
    int pid = FFLIST[frame_index].pid;
    FFLIST[frame_index].is_free = 1;
    
    if (pid == -1) {
        free_frames.frames[free_frames.count++] = frame_index;
    } else {
        process_free_frames[pid].frames[process_free_frames[pid].count++] = frame_index;
    }
}

void remove_from_free_list(int frame_index, int list_type, int process_id) {
    FrameList *list;
    
    if (list_type == 0) { 
        list = &free_frames;
    } else { 
        list = &process_free_frames[process_id];
    }
    int idx = -1;
    for (int i = 0; i < list->count; i++) {
        if (list->frames[i] == frame_index) {
            idx = i;
            break;
        }
    }
    if(idx != -1) {
        for (int i = idx; i < list->count - 1; i++) {
            list->frames[i] = list->frames[i + 1];
        }
        list->count--;
    }
}

int get_frame_from_list(FrameList *list) {
    if (list->count == 0) {
        return -1;
    }
    int frame = list->frames[list->count - 1];
    list->count--;
    return frame;
}





int LRU_Replacement(Process* p){
    int min_history = 0xFFFF;
    int min_index = 10;
    for(int i=10;i<PAGES_PER_PROCESS;i++){
        if(IS_VALID(p->page_table[i].entry)){
            if(p->page_table[i].history < min_history){
                min_history = p->page_table[i].history;
                min_index = i;
            }
        }
    }

    return min_index;
}

void find_free_frame(Process* p, int page_index, int process_number) {
    int frame = -1;
    
    // Attempt 1: Check if we already have a free frame for this specific page
    for (int i = 0; i < process_free_frames[process_number].count; i++) {
        int candidate = process_free_frames[process_number].frames[i];
        if (FFLIST[candidate].page_number == page_index) {
            frame = candidate;
            remove_from_free_list(frame, 1, process_number);
            
            #ifdef VERBOSE 
                printf("\t\tAttempt 1: Page found in free frame %d\n", frame);
            #endif
            attempts[process_number][0]++;
            break;
        }
    }
    
    // Attempt 2: Get a completely free frame (owned by no process)
    if (frame == -1 && free_frames.count > 0) {
        for(int i=TOTAL_FRAMES-1;i>=0;i--){
            if(FFLIST[i].is_free){
                frame = i;
                break;
            }
        }
        remove_from_free_list(frame, 0, -1);
        #ifdef VERBOSE 
            printf("\t\tAttempt 2: Free frame %d owned by no process found\n", frame);
        #endif
        attempts[process_number][1]++;
    }
    
    // Attempt 3: Get any free frame owned by this process
    if (frame == -1 && process_free_frames[process_number].count > 0) {
        frame = get_frame_from_list(&process_free_frames[process_number]);
        remove_from_free_list(frame, 1, process_number);
        int prev_page = FFLIST[frame].page_number;
        #ifdef VERBOSE 
            printf("\t\tAttempt 3: Own page %d found in free frame %d\n", prev_page, frame);
        #endif
        attempts[process_number][2]++;
    }
    
    // Attempt 4: Get a free frame from any other process
    if (frame == -1) {
        int total_free_frames = 0;
        for (int i = 0; i < n; i++) {
            if (i != process_number) {
                total_free_frames += process_free_frames[i].count;
            }
        }

        int rand_index = rand() % total_free_frames;
        int cumulative_sum = 0;
        for (int i = 0; i < n; i++) {
            if (i != process_number && process_free_frames[i].count > 0) {
                cumulative_sum += process_free_frames[i].count;

                if (rand_index < cumulative_sum) {
                    // This process is selected
                    int idx = rand_index - (cumulative_sum - process_free_frames[i].count);
                    frame = process_free_frames[i].frames[idx];
                    remove_from_free_list(frame, 1, i);
                    break;
                }
            }
        }
        int owner = FFLIST[frame].pid;
        
        #ifdef VERBOSE 
            printf("\t\tAttempt 4: Free frame %d owned by Process %d chosen\n", frame, owner);
        #endif
        attempts[process_number][3]++;
    }
    
    // Update the frame information
    FFLIST[frame].is_free = 0;
    FFLIST[frame].pid = process_number;
    FFLIST[frame].page_number = page_index;
    p->page_table[page_index].entry = frame;
    SET_VALID(p->page_table[page_index].entry);
}




void load_page(Process* p, int page_index,int process_number) {
    page_faults[process_number]++;

    if(NFF>NFFMIN){
        #ifdef VERBOSE
            printf("\tFault on Page %d: Free frame %d found\n",page_index,TOTAL_FRAMES-NFF);
        #endif
        int frame = free_frames.frames[0];
        free_frames.count--;
        for(int i=0;i<free_frames.count;i++){
            free_frames.frames[i] = free_frames.frames[i+1];
        }
        //remove_from_free_list(frame, 0, -1);
        assert(frame >= 0);
        assert(frame<TOTAL_FRAMES);
        p->page_table[page_index].entry = frame;
        SET_VALID(p->page_table[page_index].entry);
        FFLIST[frame].pid = process_number;
        FFLIST[frame].page_number = page_index;
        FFLIST[frame].is_free = 0;
        NFF--;
    }else{
        page_replacements[process_number]++;
        int victim_page = LRU_Replacement(p);
        int frame = p->page_table[victim_page].entry & 0x3FFF;
        assert(frame >= 0);
        assert(frame<TOTAL_FRAMES);
        int history = p->page_table[victim_page].history;

        #ifdef VERBOSE
            printf("\tFault on Page %d: To replace Page %d at Frame %d [history = %d]\n",page_index,victim_page,frame,history);
        #endif
        

        find_free_frame(p,page_index,process_number);
        // Update the page table entry of victim page
        p->page_table[victim_page].entry = 0;
        p->page_table[victim_page].history = 0;
        FFLIST[frame].is_free = 1;
        add_to_free_list(frame);
    }
}

void remove_process(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;

    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (IS_VALID(p->page_table[i].entry)) {
            int frame = p->page_table[i].entry & 0x3FFF;
            assert(frame >= 0);
            assert(frame < TOTAL_FRAMES);
            CLEAR_VALID(p->page_table[i].entry);

            FFLIST[frame].is_free = 1;
            FFLIST[frame].pid = -1;
            FFLIST[frame].page_number = -1;

            free_frames.frames[free_frames.count++] = frame;
            NFF++;
        }
    }
    process_free_frames[process_number].count = 0;

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
            p->page_table[i].history >>= 1;
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
                printf("+++ Process %d: Search %d\n", curr_process,p->searches_done + 1);
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
    FILE* fp = fopen("search.txt", "r");
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
            int frame = TOTAL_FRAMES - NFF;
            assert(frame >= 0);
            assert(frame < TOTAL_FRAMES);
            p->page_table[j].entry = frame;
            p->page_table[j].history = 0xFFFF;
            SET_VALID(p->page_table[j].entry);
            FFLIST[frame].pid = i;
            FFLIST[frame].page_number = j;
            FFLIST[frame].is_free = 0;
            
            NFF--;
        }
        queue_enqueue(&ready_queue, i);
    }
    
    fclose(fp);
    printf("+++ Simulation data read from file\n");
}

void init_fflist(){
    
    NFF = TOTAL_FRAMES;
   
    for(int i=0;i<TOTAL_FRAMES;i++){
        FFLIST[i].is_free = 1;
        FFLIST[i].pid =-1;
        FFLIST[i].page_number =-1;
    }

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
        printf("%4d  %10d  %7d (%6.2f%%)  %7d (%6.2f%%)   %3d + %3d + %3d + %3d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
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
    printf("\nTotal     %10d  %7d (%6.2f%%)  %7d (%6.2f%%)   %3d + %3d + %3d + %3d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n",
           total_accesses, total_faults, total_fault_percent, total_replacements, total_replacement_percent,
           total_attempts[0], total_attempts[1], total_attempts[2], total_attempts[3],
           total_attempt_percent[0], total_attempt_percent[1], total_attempt_percent[2], total_attempt_percent[3]);
}

int main() {
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
    free_frames.count = 0;
    for(int i=0;i<n;i++){
        process_free_frames[i].count = 0;
    }
    for(int i=0;i<TOTAL_FRAMES;i++){
        if(FFLIST[i].is_free){
            free_frames.frames[free_frames.count++] = i;
        }
    }

    run_simulation();
    print_results();

    for (int i = 0; i < n; i++) {
        free(processes[i].search_indices);
        free(processes[i].page_table);
    }
    free(processes);

    return 0;
}