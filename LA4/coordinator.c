#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>


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
  
  return 0;
}

