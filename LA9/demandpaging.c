#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PROCESSES 500
#define MAX_SEARCHES 100
#define PAGES_PER_PROCESS 2048
#define TOTAL_FRAMES 12288
#define ESSENTIAL_PAGES 10

#define SET_VALID(x) ((x) |= (1<<15))
#define CLEAR_VALID(x) ((x) &= ~(1<<15))
#define IS_VALID(x) ((x) & (1<<15))

typedef struct {
    int front;
    int rear;
    int items[TOTAL_FRAMES];
    int size;
} FrameQueue;

typedef struct{
  int size; //size of array A
  int *search_indices;
  unsigned short *page_table;
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

void frame_queue_init(FrameQueue* q) {
    q->front = q->rear = -1;
    q->size = 0;
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        q->items[i] = i;
    }
    q->rear = TOTAL_FRAMES - 1;
    q->size = TOTAL_FRAMES;
}

int frame_queue_dequeue(FrameQueue* q) {
    if (q->size == 0) {
        fprintf(stderr, "No free frames available\n");
        exit(1);
    }
    
    int frame = q->items[q->front];
    
    if (q->size == 1) {
        q->front = q->rear = -1;
    } else {
        q->front = (q->front + 1) % TOTAL_FRAMES;
    }
    
    q->size--;
    return frame;
}

void frame_queue_enqueue(FrameQueue* q, int frame) {
    if (q->size == TOTAL_FRAMES) {
        fprintf(stderr, "Free frames queue is full\n");
        return;
    }
    
    if (q->rear == -1) {
        q->front = q->rear = 0;
    } else {
        q->rear = (q->rear + 1) % TOTAL_FRAMES;
    }
    
    q->items[q->rear] = frame;
    q->size++;
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

int n,m;
int page_faults,page_accesses,swaps,min_active_processes;
Process* processes;
Queue ready_queue,swapped_out_queue;
FrameQueue free_frames_queue;

bool load_page(Process* p, int page_index) {
    page_faults++;

    
    if (free_frames_queue.size > 0) {
        int frame = frame_queue_dequeue(&free_frames_queue);
        p->page_table[page_index] = frame;
        SET_VALID(p->page_table[page_index]);
        
        return true;
    }

    return false;
}

void swap_out_process(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;


    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (IS_VALID(p->page_table[i])) {
            int frame = p->page_table[i] & 0x3FFF;
            CLEAR_VALID(p->page_table[i]);
            frame_queue_enqueue(&free_frames_queue, frame);
        }
    }

    if (p->searches_done < m) {
        queue_enqueue(&swapped_out_queue, process_number);

        printf("+++ Swapping out process %4d  [%d active processes]\n", process_number, ready_queue.size);
        if (ready_queue.size < min_active_processes) {
            min_active_processes = ready_queue.size;
        }
        swaps++;
    }
}

bool binary_search(int process_number) {
    Process *p = &processes[process_number];
    int search_key = p->search_indices[p->searches_done];
    int L = 0, R = p->size - 1;

    while (L < R) {
        int M = (L + R) / 2;
        int page_index = 10 + M / 1024;
        page_accesses++;

        if (!IS_VALID(p->page_table[page_index])) {
            if (!load_page(p, page_index)) {
                swap_out_process(process_number);
                return false;
            }
        }

        if (search_key <= M) {
            R = M;
        } else {
            L = M + 1;
        }
    }
    p->searches_done++;
    return true;
}

void swap_in_process() {
    if (free_frames_queue.size < ESSENTIAL_PAGES) {
        return;
    }
    
    if (queue_is_empty(&swapped_out_queue)) {
        return;
    }
    
    int process_number = queue_dequeue(&swapped_out_queue);
    Process* p = &processes[process_number];

    
    for (int i = 0; i < ESSENTIAL_PAGES; i++) {
        int frame = frame_queue_dequeue(&free_frames_queue);
        p->page_table[i] = frame;
        SET_VALID(p->page_table[i]);
    }
    
    printf("+++ Swapping in process %5d  [%d active processes]\n", process_number, ready_queue.size + 1);
    p->is_active = true;
    
    #ifdef VERBOSE
        printf("\tSearch %d by Process %d\n", p->searches_done + 1, process_number);
    #endif
    
    if (binary_search(process_number)) {
        if (p->searches_done < m) {
            queue_enqueue(&ready_queue, process_number);
        } else {
            swap_out_process(process_number);
            swap_in_process();
        }
    }
}

void run_simulation() {
    printf("+++ Kernel data initialized\n");
    min_active_processes = n;
    
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
                printf("\tSearch %d by Process %d\n", p->searches_done + 1, curr_process);
            #endif
            
            if (binary_search(curr_process)) {
                if (p->searches_done < m) {
                    queue_enqueue(&ready_queue, curr_process);
                } else {
                    swap_out_process(curr_process);
                    swap_in_process();
                }
            }
        }
    }
}

void print_results() {
    printf("\n+++ Page access summary\n");
    printf("Total number of page accesses = %d\n", page_accesses);
    printf("Total number of page faults = %d\n", page_faults);
    printf("Total number of swaps = %d\n", swaps);
    printf("Degree of multiprogramming = %d\n", min_active_processes);
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
        p->page_table = calloc(PAGES_PER_PROCESS, sizeof(unsigned short));
        
        fscanf(fp, "%d", &p->size);
        p->search_indices = (int*)malloc(m * sizeof(int));
        
        for (int j = 0; j < m; j++) {
            fscanf(fp, "%d", &p->search_indices[j]);
        }
        
        for (int j = 0; j < ESSENTIAL_PAGES; j++) {
            int frame = frame_queue_dequeue(&free_frames_queue);
            p->page_table[j] = frame;
            SET_VALID(p->page_table[j]);
        }

        queue_enqueue(&ready_queue, i);
    }
    
    fclose(fp);
    printf("+++ Simulation data read from file\n");
}

int main() {
    queue_init(&ready_queue);
    queue_init(&swapped_out_queue);
    frame_queue_init(&free_frames_queue);

    page_faults = 0;
    page_accesses = 0;
    swaps = 0;

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