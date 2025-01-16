#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

const char* PID_TXT_FILE = "childpid.txt";
const char* DUM_TXT_FILE = "dummycpid.txt";
enum Status{
    PLAYING,
    CATCHMADE,
    CATCHMISSED,
    OUTOFGAME
} currStat;
pid_t nextpid=-1;
int ch;

/* Signal Handlers */

void attemptCatch(int){
    int num = rand()%10;
    pid_t ppid = getppid();
    if(num<=1){
        currStat = CATCHMISSED;
        kill(ppid,SIGUSR2);
    }
    else{
        currStat = CATCHMADE;
        kill(ppid,SIGUSR1);
    }
}

void printStatus(int){
    switch(currStat){
        case PLAYING:
            printf("....\t");
            break;
        case OUTOFGAME:
            printf("    \t");
            break;
        case CATCHMADE:
            printf("CATCH\t");
            currStat = PLAYING;
            break;
        case CATCHMISSED:
            printf("MISS\t");
            currStat = OUTOFGAME;
            break;
    }
    fflush(stdout);

    if(nextpid>0){
        kill(nextpid,SIGUSR1);
    }
    else{
        FILE* dfp = fopen(DUM_TXT_FILE,"r");
        pid_t dpid;
        fscanf(dfp,"%d",&dpid);
        fclose(dfp);
        kill(dpid,SIGINT);
    }
}

void exitGame(int){
    if(currStat==PLAYING){
        printf("\n+++ Child %d: Yay! I am the winner!\n", ch);
        fflush(stdout);
    }
    exit(0);
}

int main(int argc, const char* argv[]){
    if(argc<2){
        printf("Usage: %s [n]\n",argv[0]);
        exit(0);
    }
    sleep(2);
    srand((unsigned int)getpid());

    ch = atoi(argv[1]);
    currStat = PLAYING;

    FILE *fp = fopen(PID_TXT_FILE,"r");
    int n;
    fscanf(fp,"%d",&n);
    for(int j=1;j<=n;j++){
        fscanf(fp,"%d",&nextpid);
        if(j==ch%n+1) break;
    }
    if(ch==n) nextpid=-1;
    fclose(fp);

    signal(SIGUSR1,printStatus);
    signal(SIGUSR2,attemptCatch);
    signal(SIGINT,exitGame);

    while(1){
        pause();
    }
}