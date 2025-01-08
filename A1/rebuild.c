#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

const char* DAG_TXT_FILE = "foodep.txt";
const char* VIS_TXT_FILE = "done.txt";
const int MAXN = 1000;

// Check if the foodule is already rebuilt
bool isVisited(int v, int n){
    FILE* fp = fopen(VIS_TXT_FILE,"r");
    char* vis = (char*)malloc((n+1)*sizeof(char));
    fscanf(fp,"%s",vis);
    fclose(fp);
    return (vis[v-1]=='1');
}

// Mark the foodule as rebuilt
void markVisited(int v, int n){
    // Read Visited Array
    FILE* fp = fopen(VIS_TXT_FILE,"r");
    char* vis = (char*)malloc((n+1)*sizeof(char));
    fgets(vis,n+1,fp);
    fclose(fp);
    
    // Update Visited Array
    vis[v-1]='1';
    fp = fopen(VIS_TXT_FILE,"w");
    fputs(vis,fp);
    fclose(fp);
    free(vis);
}

int main(int argc, const char* argv[]){
    if(argc<2){
        printf("Usage: %s [u]\n",argv[0]);
        exit(0);
    }
    
    int u = atoi(argv[1]); // Foodule to rebuild

    int n;  // No. of foodules
    FILE* fp = fopen(DAG_TXT_FILE,"r");
    fscanf(fp,"%d",&n);

    if(u<=0 || u>n){
        printf("foodmodule number should be in the range 1-%d",n);
        fclose(fp);
        exit(0);
    }

    if(argc==2){
        // Root process -- initialize visited array
        FILE* wp = fopen(VIS_TXT_FILE,"w");
        char* vis = (char*)malloc((n+1)*sizeof(char));
        for(int i=0;i<n;i++) vis[i]='0';
        fputs(vis,wp);
        free(vis);
        fclose(wp);
    }
    // Find the dependency list
    char dep[MAXN];
    for(int i=0;i<=n;i++){
        fgets(dep,MAXN-1,fp);
        if(i==u) break;
    }
    fclose(fp);

    // Split at ':'
    char* tok = strtok(dep,":");
    tok = strtok(NULL,":");

    // Split at ' '
    tok = strtok(tok," ");
    int ct=0, v[MAXN];
    while(tok!=NULL){
        if(tok[0]!=' ' && tok[0]!='\n'){
            v[ct]=atoi(tok);
            ct++;
        }
        tok = strtok(NULL," ");
    }

    // DFS Traversal
    for(int i=0;i<ct;i++){
        if(isVisited(v[i],n)) continue;

        pid_t cpid = fork();

        if(cpid){
            // Wait for child process to finish
            wait(NULL);
        }
        else{
            // Execute rebuild with the dependency foodule
            char num[10];
            sprintf(num,"%d",v[i]);
            execlp("./rebuild","./rebuild",num,"NOROOT",NULL);
        }
    }
    markVisited(u,n);

    printf("foo%d rebuilt",u);
    if(ct){
        printf(" from");
        for(int i=0;i<ct;i++){
            printf(" foo%d%c",v[i],((i==ct-1)?' ':','));
        }
    }
    printf("\n");
}