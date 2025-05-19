#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

#define MAX_UID 65535
char* uid_to_login[MAX_UID] = { NULL };
int file_count = 0;

void build_uid_map(){
    FILE* fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        perror("Error opening /etc/passwd");
        exit(1);
    }
    char line[1024];
    while(fgets(line,sizeof(line),fp)){
        size_t len = strlen(line);
        if(len > 0 && line[len-1] == '\n'){
            line[len-1] = '\0';
        }
        char* user_name = strtok(line, ":");
        strtok(NULL,":");
        char* uid_str = strtok(NULL, ":");
        if(user_name && uid_str){
            int uid = atoi(uid_str);
            if(uid>=0 && uid<MAX_UID){
                uid_to_login[uid] = strdup(user_name);
            }
        }
    }
    fclose(fp);
}

int has_extension(const char *filename, const char *ext) {
    const char *dot = strrchr(filename, '.');
    return (dot && strcmp(dot + 1, ext) == 0);
}

void traverse(const char* path, const char* ext){
    DIR* dir = opendir(path);
    if(dir == NULL){
        printf("Error opening directory %s: %s\n", path, strerror(errno));
        exit(1);
    }
    struct dirent* entry;

    while((entry=readdir(dir))){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name, "..")==0){
            continue;
        }
        char fullpath[4096];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        struct stat sb;
        if(stat(fullpath,&sb) == -1){
            perror(fullpath);
            exit(1);
        }

        if(S_ISDIR(sb.st_mode)){
            traverse(fullpath, ext);
        }else{
            if(S_ISREG(sb.st_mode) && has_extension(entry->d_name,ext)){
                const char* owner = (sb.st_uid < MAX_UID && uid_to_login[sb.st_uid]) ? uid_to_login[sb.st_uid]:"unknown";
                printf("%-10d: %-20s %-20ld %s\n", ++file_count, owner, sb.st_size, fullpath);
            }
        }

    }
    closedir(dir);
}


int main(int argc,char* argv[]){
    if(argc!=3){
        printf("Usage: %s <directory> <file_extension>\n", argv[0]);
        exit(1);
    }
    build_uid_map();
    printf("NO        : OWNER               SIZE                 NAME\n");
    printf("--        : -----               ----                 ----\n");
    traverse(argv[1], argv[2]);

    printf("+++ %d files match the extension %s\n", file_count, argv[2]);

    exit(0);

}