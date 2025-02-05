#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, const char* argv[]){
    int nf = 1;
    if(argc > 1){
        nf = atoi(argv[1]);
    }

    key_t key = ftok("/shm", 65);
    int shmid = shmget(key, 0, 0);
    
    for(int i=0;i<nf;i++){
        pid_t pid = fork();
        if(pid<0){
            perror("follower: fork");
            exit(1);
        }
        if(!pid){
            int *shm = (int*)shmat(shmid, 0, 0);
            if(shm[1] == shm[0]){
                printf("follower error: all followers have already joined\n");
                shmdt(shm);
                exit(0);
            }
            srand(getpid());
            shm[1]++;
            int flnum = shm[1];
            printf("follower %d joins\n", flnum);
            while(1){
                while(abs(shm[2]) != flnum);
                if(shm[2]<0){
                    shm[2] = ((flnum!=shm[0])?-(flnum+1):0);
                    printf("follower %d leaves\n", flnum);
                    shmdt(shm);
                    exit(0);
                }
                shm[flnum+3] = 1+rand()%9;
                shm[2] = ((flnum!=shm[0])?(flnum+1):0);
            }
        }
    }

    for(int i=0;i<nf;i++){
        wait(NULL);
    }
    exit(0);
}