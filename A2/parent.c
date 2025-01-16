#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

const char* PID_TXT_FILE = "childpid.txt";
const char* DUM_TXT_FILE = "dummycpid.txt";
const char* CHILD = "./child";
const char* DUMMY = "./dummy";

bool* isPlaying = NULL;
pid_t* childPID = NULL;
int n;
volatile sig_atomic_t caught;

// Signal handler
void sigHandler (int sig){
    switch(sig){
        case SIGUSR1:
            caught=1;
            break;
        case SIGUSR2:
            caught=0;
            break;
    }
}

// Helper function
int next(int i){
    int j = (i+1)%n;
    for(;!isPlaying[j];j=(j+1)%n);
    return j;
}

/* Printing functions */

void printNums(){
    printf("\n\t");
    for(int j=1;j<=n;j++) printf("%3d\t",j);
    fflush(stdout);
}

void printLine(){
    printf("\n+");
    for(int j=0;j<8*(n+1);j++) printf("-");
    printf("+");
    fflush(stdout);
}

int main(int argc, const char* argv[]){
    if(argc<2){
        printf("Usage: %s [n]\n",argv[0]);
        exit(0);
    }

    n = atoi(argv[1]);
    isPlaying = (bool*)malloc(n*sizeof(bool));
    childPID = (pid_t*)malloc(n*sizeof(pid_t));

    FILE *fp = fopen(PID_TXT_FILE,"w");
    fprintf(fp,"%d\n",n);
    fflush(fp);

    // Create child processes
    for(int i=0;i<n;i++){
        isPlaying[i]=true;
        childPID[i] = fork();

        if(childPID[i]){
            fprintf(fp,"%d\n",childPID[i]);
            fflush(fp);
        }
        else{
            char num[20];
            sprintf(num,"%d",i+1);
            execlp(CHILD,CHILD,num,NULL);
        }
    }

    printf("Parent: %d child processes created\n",n);
    fclose(fp);
    int t = 3;
    printf("Parent: Waiting %d seconds for child processes to read child database\n",t);
    sleep(t);

    printNums();
    printLine();

    signal(SIGUSR1,sigHandler);
    signal(SIGUSR2,sigHandler);

    int ct=n;
    int curr=-1;
    while(ct>1){
        curr = next(curr);
        caught=-1;
        kill(childPID[curr],SIGUSR2);

        if(caught==-1) pause(); // The process pauses to receive signal and doesn't if signal is already received
        
        if(caught==0){
            fflush(stdout);
            isPlaying[curr]=false;
            ct--;
        }
        pid_t dpid = fork();
        if(dpid){
            FILE* dfp = fopen(DUM_TXT_FILE,"w");
            fprintf(dfp,"%d",dpid);
            fflush(dfp);
            fclose(dfp);
            
            printf("\n|\t");
            fflush(stdout);

            kill(childPID[0],SIGUSR1);
            waitpid(dpid,NULL,0);
            
            printf(" |");
            printLine();
        }
        else{
            execlp(DUMMY,DUMMY,NULL);
        }
    }

    printNums();

    for(int i=0;i<n;i++) kill(childPID[i],SIGINT);
}