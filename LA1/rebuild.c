#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#define VISITED "done.txt"
#define DAG "foodep.txt"
void process_module(char* module){
  FILE* fp=fopen(DAG,"r");
  if(fp==NULL){
    fprintf(stderr,"Error opening file %s\n",DAG);
    exit(1);
  }
  char line[256],curr_module[8],child[100];
  int found=0,child_count=0;
  fgets(line,256,fp);
  //printf("%s\n",line);
  while(1){
    fgets(line,256,fp);
    if(feof(fp)){
      break;
    }
    //printf("%s\n",line);
    char* curr=line;
    sscanf(curr,"%s",curr_module);
    int len=strlen(curr_module);
    curr+=len+1;
    curr_module[len-1]='\0';
    //printf("%s\n",curr_module);
    if(!strcmp(curr_module,module)){
      found=1;
      //printf("MATCH\n");
      while(1){
        if(curr[0]=='\0'){
          break;
        }
        char dep[8];
        sscanf(curr,"%s",dep);
        //printf("%s\n",dep);
        // if(dep[0]=='\0'){
        //   break;
        // }
        curr+=strlen(dep)+1;
        FILE* vis=fopen(VISITED,"r+");
        int ndep=atoi(dep);
        fseek(vis,ndep,SEEK_SET);
        char c=fgetc(vis);
        //printf("%c\n",c);
        if(c=='0'){
          pid_t cpid=fork();
          if(cpid==0){
            fseek(vis,ndep,SEEK_SET);
            fprintf(vis,"%c",'1');
            fclose(vis);
            execlp("./rebuild","./rebuild",dep,"x",NULL);
          }
          else{
            waitpid(cpid,NULL,0);
          }
        }
        child[child_count++]=atoi(dep);
      }
      printf("foo%d rebuilt ",atoi(module));
      if(child_count>0){
        printf("from ");
        for(int i=0;i<child_count;i++){
          if(i!=child_count-1)printf("foo%d, ",child[i]);
          else printf("foo%d ",child[i]);
        }
        
      }
      printf("\n");
      break;
    }
  }
  fclose(fp);
  if(!found){
    fprintf(stderr,"Module %s not found\n",module);
    exit(1);
  }
}





int main(int argc,char* argv[]){
  if(argc<2){
    fprintf(stderr,"Usage: %s <command> <args>\n",argv[0]);
    fprintf(stderr,"Provide Module name as argument\n");
    exit(1);
  }


  if(argc==2){
    FILE* vis=fopen(VISITED,"w");
    FILE* fp=fopen(DAG,"r");
    char tmp[8];
    if(fp==NULL){
      fprintf(stderr,"Error opening file %s\n",DAG);
      exit(1);
    }
    fgets(tmp,8,fp);
    int n=atoi(tmp);
    char* str=(char*)malloc((n+1)*sizeof(char));
    for(int i=0;i<=n;i++){
      if(i==atoi(argv[1]))str[i]='1';
      else str[i]='0';
    }
    fprintf(vis,"%s",str);
    fclose(fp);
    fclose(vis);
  }

  process_module(argv[1]);
  return 0;
}
