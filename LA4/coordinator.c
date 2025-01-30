#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<time.h>
#include "boardgen.c"
#define NUM_BLOCKS 9


void displayHelp() {
  printf("Commands supported\n");
  printf("\tn          Start new game\n");
  printf("\tp b c d    Put digit d [1-9] at cell c [0-8] of block b [0-8]\n");
  printf("\ts          Show solution\n");
  printf("\th          Print this help message\n");
  printf("\tq          Quit\n\n");

  printf("Numbering scheme for blocks and cells\n");
  printf("\t+---+---+---+\n");
  printf("\t| 0 | 1 | 2 |\n");
  printf("\t+---+---+---+\n");
  printf("\t| 3 | 4 | 5 |\n");
  printf("\t+---+---+---+\n");
  printf("\t| 6 | 7 | 8 |\n");
  printf("\t+---+---+---+\n");
}
int main(){
  int pfds[NUM_BLOCKS][2];
  for(int i=0;i<NUM_BLOCKS;i++){
      if(pipe(pfds[i])==-1){
          perror("pipe failed");
          exit(1);
      }
  }
  char geometry[NUM_BLOCKS][20] = {
        "25x8+300+50", "25x8+650+50", "25x8+1000+50",
        "25x8+300+280", "25x8+650+280", "25x8+1000+280",
        "25x8+300+550", "25x8+650+550", "25x8+1000+550"
  };
  for(int i=0;i<NUM_BLOCKS;i++){
      pid_t pid=fork();
      if(pid==-1){
          perror("fork failed");
          exit(1);
      }
      if(pid==0){
        //   for(int j=0;j<NUM_BLOCKS;j++){
        //       if(j!=i || j/3!=i/3 || j%3!=i%3){
        //           close(pfds[j][0]);
        //           close(pfds[j][1]);
        //       }
        //   }
           char block_no[10];
           snprintf(block_no, sizeof(block_no), "%d", i);
           char block_title[20]="Block ";
           strcat(block_title,block_no);
           char bfd_in[10], bfd_out[10],rd1_out[10],rd2_out[10],cd1_out[10],cd2_out[10];
          snprintf(bfd_in, sizeof(bfd_in), "%d", pfds[i][0]);
          snprintf(bfd_out, sizeof(bfd_out), "%d", pfds[i][1]);

          if(i%3==0){
              snprintf(rd1_out, sizeof(rd1_out), "%d", pfds[i+1][1]);
              snprintf(rd2_out, sizeof(rd2_out), "%d", pfds[i+2][1]);
          }
          else if(i%3==1){
              snprintf(rd1_out, sizeof(rd1_out), "%d", pfds[i-1][1]);
              snprintf(rd2_out, sizeof(rd2_out), "%d", pfds[i+1][1]);
          }
          else if(i%3==2){
              snprintf(rd1_out, sizeof(rd1_out), "%d", pfds[i-1][1]);
              snprintf(rd2_out, sizeof(rd2_out), "%d", pfds[i-2][1]);
          }

          if(i/3==0){
              snprintf(cd1_out, sizeof(cd1_out), "%d", pfds[i+3][1]);
              snprintf(cd2_out, sizeof(cd2_out), "%d", pfds[i+6][1]);
          }
          else if(i/3==1){
              snprintf(cd1_out, sizeof(cd1_out), "%d", pfds[i-3][1]);
              snprintf(cd2_out, sizeof(cd2_out), "%d", pfds[i+3][1]);
          }
          else if(i/3==2){
              snprintf(cd1_out, sizeof(cd1_out), "%d", pfds[i-3][1]);
              snprintf(cd2_out, sizeof(cd2_out), "%d", pfds[i-6][1]);
          }

          execlp("xterm", "xterm", "-T", block_title, "-fa", "Monospace", "-fs", "12", "-geometry", geometry[i],
                   "-bg", "#300a24","-fg","white","-e", "./block", block_no, bfd_in, bfd_out,rd1_out,rd2_out,cd1_out,cd2_out,NULL);
          perror("execlp");
          exit(1);
      }
  }
  displayHelp();
  int A[9][9],S[9][9];
  int fd1copy;
  while(1){
      char ch=' ';
      while(ch=='\n' || ch==' '){
          printf("Foodoku> ");
          ch=getchar();
      }
      if(ch=='h'){
        displayHelp();
      }
      else if(ch=='n'){
        newboard(A,S);
        // for(int i=0;i<9;i++){
        //     for(int j=0;j<9;j++){
        //         printf("%d ",A[i][j]);
        //     }
        //     printf("\n");
        // }
        fd1copy=dup(1);
        close(1);
        for(int i=0;i<NUM_BLOCKS;i++){
            dup(pfds[i][1]);
            printf("n");
            //int tmp[3][3];
            int rowst=(i/3)*3;
            int rowen=rowst+3;
            int colst=(i%3)*3;
            int colen=colst+3;
            //int i_=0,j_=0;
            for(int j=rowst;j<rowen;j++){
                for(int k=colst;k<colen;k++){
                    printf("%d ",A[j][k]);
                }
            }
            fflush(stdout);
            close(1);
        }
        dup(fd1copy);
        close(fd1copy);
      }
      else if(ch=='p'){
        int arg[3];
        for(int i=0;i<3;i++){
            int tmp;
            scanf("%d",&tmp);
            arg[i]=tmp;
            printf("%d\n",tmp);
        }
        if(arg[0]<0 || arg[0]>8 || arg[1]<0 || arg[1]>8 || arg[2]<1 || arg[2]>9){
            printf("Error: Parameters not in range.Type 'h' for more details\n");
            ch=getchar();
            continue;
        }
        fd1copy=dup(1);
        close(1);
        dup(pfds[arg[0]][1]);
        printf("p %d %d\n",arg[1],arg[2]);
        fflush(stdout);
        close(1);
        dup(fd1copy);
        close(fd1copy);
      }
      else if(ch=='s'){
        fd1copy=dup(1);
        close(1);
        for(int i=0;i<NUM_BLOCKS;i++){
            dup(pfds[i][1]);
            printf("n");
            int rowst=(i/3)*3;
            int rowen=rowst+3;
            int colst=(i%3)*3;
            int colen=colst+3;
            for(int j=rowst;j<rowen;j++){
                for(int k=colst;k<colen;k++){
                    printf("%d ",S[j][k]);
                }
            }
            fflush(stdout);
            close(1);
        }
        dup(fd1copy);
        close(fd1copy);
      }
      else if(ch=='q'){
        fd1copy=dup(1);
        close(1);
        for(int i=0;i<NUM_BLOCKS;i++){
            dup(pfds[i][1]);
            printf("q");
            fflush(stdout);
            close(1);
        }
        dup(fd1copy);
        close(fd1copy);
        break;
      }
      else{
          printf("Invalid Command.Type 'h' for available commands\n");
      }
      ch=getchar();
  }
  for (int i = 0; i < NUM_BLOCKS; i++) {
    close(pfds[i][0]);
    close(pfds[i][1]);
  }

  for (int i = 0; i < NUM_BLOCKS; i++) {
    wait(NULL);
  }

  return 0;
}

