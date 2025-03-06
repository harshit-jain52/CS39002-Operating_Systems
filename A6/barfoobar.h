#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <signal.h>

#define MUTEX_PATH "a6_mutex"
#define COOK_PATH "a6_cook"
#define WAITER_PATH "a6_waiter"
#define CUSTOMER_PATH "a6_customer"
#define SHM_PATH "a6_shm"
#define PROJ_ID 'F'

#define MAX_CUSTOMERS 200
#define WAITERS 5
#define MAX_CUST_PER_WAITER (MAX_CUSTOMERS/WAITERS)
#define COOKS 2
#define TABLES 10
#define MAX_TIME 240
#define SHM_SIZE 2000
#define msleep(x) usleep((x)*100000)

void down(int semid, int i){
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = -1; // Acquire
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1){
        perror("down: semop");
        exit(1);
    }
}

void up(int semid, int i){
    struct sembuf sb;
    sb.sem_num = i;
    sb.sem_op = 1; // Release
    sb.sem_flg = 0;

    if (semop(semid, &sb, 1) == -1){
        perror("up: semop");
        exit(1);
    }
}

void timestamp(int min){
    int hr = min/60;
    min %= 60;
    printf("[%02d:%02d]: ", 11+hr, min);
}

void log_message(int min, const char *format, ...) {
    timestamp(min);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

void cleanup(int signo){

    int mutex_id = semget(ftok(MUTEX_PATH, PROJ_ID), 0, 0);
    int cook_id = semget(ftok(COOK_PATH, PROJ_ID), 0, 0);
    int waiter_id = semget(ftok(WAITER_PATH, PROJ_ID), 0, 0);
    int customer_id = semget(ftok(CUSTOMER_PATH, PROJ_ID), 0, 0);
    int shmid = shmget(ftok(SHM_PATH, PROJ_ID), 0, 0);

    if (shmid != -1){
        shmctl(shmid, IPC_RMID, 0);
        printf("SHM %d removed\n", shmid);
    }

    if (mutex_id != -1){
        semctl(mutex_id, 0, IPC_RMID);
        printf("SEM %d removed\n", mutex_id);
    }

    if (cook_id != -1){
        semctl(cook_id, 0, IPC_RMID);
        printf("SEM %d removed\n", cook_id);
    }

    if (waiter_id != -1){
        semctl(waiter_id, 0, IPC_RMID);
        printf("SEM %d removed\n", waiter_id);
    }

    if (customer_id != -1){
        semctl(customer_id, 0, IPC_RMID);
        printf("SEM %d removed\n", customer_id);
    }

    if (signo == SIGSEGV){
        printf("Segmentation fault\n");
    }
    exit(0);
}