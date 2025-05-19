// #include <stdio.h>
// #include <pthread.h>
// #include <unistd.h>

// void* thread_func(void* arg) {
//   printf("Thread running...\n");
//   pthread_exit(NULL);
// }

// int main() {
//     pthread_t thread;
//     pthread_create(&thread, NULL, thread_func, NULL);

//     printf("Main thread calling pthread_exit...\n");
//     pthread_exit(NULL); // Only the main thread exits, other threads keep running.

//     printf("This will never print!\n"); // Unreachable
//     return 0;
// }

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* thread_func(void* arg) {
    sleep(1);
    printf("Thread is running\n");
    pthread_exit("Thread finished");
}

int main() {
    pthread_t thread;
    void* result;

    pthread_create(&thread, NULL, thread_func, NULL);
    //pthread_join(thread, &result);

    printf("Thread returned: %s\n", (char*)result);
    pthread_exit(NULL);
}

