#include <barfoobar.h>

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
    char meridian[2][3] = {"am", "pm"};
    int hr = START_TIME_12_HOUR + min/60;
    min %= 60;
    printf("[%02d:%02d %s]: ", (hr > 12) ? hr-12 : hr, min, meridian[(hr >= 12)]);
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