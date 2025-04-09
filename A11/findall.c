#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAXUSERS 100
#define MAXPATH 4096
#define MAXLOGINID 32

typedef struct user {
    int uid;
    char name[MAXLOGINID];
} user;

int ct = 0;
user users[MAXUSERS];
int user_count = 0;

// Convert UID to login ID
void UIDtoLOGINID(int uid, char* loginid){
    loginid[0] = '\0';

    // Check if the user is already in the cache
    for(int i = 0; i < user_count; i++){
        if(users[i].uid == uid){
            strcpy(loginid, users[i].name);
            return;
        }
    }

    // If not found in cache, read from /etc/passwd
    FILE* fp = fopen("/etc/passwd", "r");
    if(fp == NULL){
        perror("fopen");
        return;
    }
    char line[256];
    while(fgets(line, sizeof(line), fp) != NULL){
        int fileuid;
        char name[MAXLOGINID];

        char* token = strtok(line, ":"); // username
        if(token == NULL) continue;
        strcpy(name, token);

        token = strtok(NULL, ":"); // password
        if(token == NULL) continue;
        
        token = strtok(NULL, ":");  // uid
        if(token == NULL) continue;
        sscanf(token, "%d", &fileuid);

        if(fileuid == uid){
            strcpy(loginid, name);
            if(user_count < MAXUSERS){
                users[user_count].uid = uid;
                strcpy(users[user_count].name, name);
                user_count++;
            }
            break;
        }
    }
    if(strlen(loginid) == 0){
        strcpy(loginid, "unknown");
    }
    fclose(fp);
}

// Check if the file has the specified extension
bool checkext(const char* filename, const char* ext){
    const char* dot = strrchr(filename, '.');
    if(dot && strcmp(dot+1, ext) == 0){
        return true;
    }
    return false;
}

// Recursively find files with the specified extension
void findall(const char* dirname, const char* ext){
    DIR* dir = opendir(dirname);
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL){
        char path[MAXPATH];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);
        struct stat statbuf;
        stat(path, &statbuf);
        if(stat(path, &statbuf) == -1){
            perror("stat");
            continue;
        }
        if(S_ISDIR(statbuf.st_mode)){
            if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
                findall(path, ext);
            }
        } 
        else if(S_ISREG(statbuf.st_mode)){
            if(checkext(entry->d_name, ext)){
                char loginid[MAXLOGINID];
                loginid[0] = '\0';
                UIDtoLOGINID(statbuf.st_uid, loginid);
                printf("%d\t\t: %s\t\t\t%ld\t\t\t%s\n", ++ct, loginid, statbuf.st_size, path);
            }
        }   
    }

    closedir(dir);
}

int main(int argc, char* argv[]){
    if(argc!=3){
        printf("Usage: %s <dname> <ext>\n", argv[0]);
        return 1;
    }

    printf("NO\t\t: OWNER\t\t\tSIZE\t\t\tNAME\n");
    printf("--\t\t  -----\t\t\t----\t\t\t----\n");

    // Remove extra trailing slashes
    char* dir = argv[1];
    while(dir[strlen(dir)-1] == '/' && strlen(dir) > 1){
        dir[strlen(dir)-1] = '\0';
    }
        
    findall(dir, argv[2]);

    if(ct > 1) printf("+++ %d files match the extension %s\n", ct, argv[2]);
    else if(ct == 1) printf("+++ 1 file matches the extension %s\n", argv[2]);
    else printf("+++ No files match the extension %s\n", argv[2]);
}