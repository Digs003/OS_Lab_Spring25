#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "treeinfo.txt"

void process_node(char *city, int level) {
  FILE *fp = (FILE *)fopen(FILENAME, "r");
  if (fp == NULL) {
    fprintf(stderr, "Unable to open data file\n");
    exit(2);
  }
  pid_t mypid, cpid;
  mypid = getpid();

  char line[256], name[32], cc[8], cname[32], clvl[8];
  int child_cnt;
  while (1) {
    fgets(line, 256, fp);
    if (feof(fp))
      break;
    char *cp = line;
    sscanf(cp, "%s", name);
    cp += strlen(name) + 1;
    if (!strcmp(name, city)) {
      for (int i = 0; i < level; i++)
        printf("  ");
      printf("%s (%d)\n", name, mypid);
      sscanf(cp, "%s", cc);
      cp += strlen(cc) + 1;
      child_cnt = atoi(cc);
      for (int i = 0; i < child_cnt; i++) {
        sscanf(cp, "%s", cname);
        cp += strlen(cname) + 1;
        if ((cpid = fork())) {
          waitpid(cpid, NULL, 0);
        } else {
          sprintf(clvl, "%d", level + 1);
          execlp("./proctree", "./proctree", cname, clvl, NULL);
        }
      }
      break;
    }
  }
  fclose(fp);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    fprintf(stderr, "Run with a city name\n");
    exit(1);
  }
  process_node(argv[1], (argc >= 3) ? atoi(argv[2]) : 0);

  exit(0);
}
