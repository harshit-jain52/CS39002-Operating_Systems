#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

bool vis[1005];

int main(int argc, const char* argv[]){
    srand(getpid());
    memset(vis, 0, sizeof(vis));

    int n = 10;
    if(argc > 1){
        n = atoi(argv[1]);
        if(n>100){
            printf("n should be <= 100\n");
            exit(1);
        }
    }

    key_t key = ftok("/shm", 65);
    int shmid = shmget(key, (n+4)*sizeof(int), 0777|IPC_CREAT|IPC_EXCL);
    // printf("Shared memory: %d\n", shmid);
    if(shmid<0){
        perror("leader: shmget");
        exit(1);
    }

    int* shm = (int*)shmat(shmid, 0, 0);
    shm[0] = n;
    shm[1] = 0;
    shm[2] = 0;

    while(shm[1] != n);
    // printf("Leader's wait is over!\n");

    while(1){
        shm[3] = 1+rand()%99;
        shm[2] = 1;
        while(shm[2] != 0);
        int sum=0;
        for(int j=0;j<=n;j++){
            printf("%d ", shm[j+3]);
            if(j == n) printf("= ");
            else printf("+ ");
            sum += shm[j+3];
        }
        printf("%d\n", sum);
        if(vis[sum]) break;
        vis[sum] = true;
    }

    shm[2] = -1;
    while(shm[2] != 0);
    
    shmdt(shm);
    shmctl(shmid, IPC_RMID, 0);
    exit(0);
}