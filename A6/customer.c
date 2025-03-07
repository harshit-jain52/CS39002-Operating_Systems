#include <barfoobar.h>
int mutex_id, cook_id, waiter_id, customer_id;
int shmid;
const char* customer_file = "customers.txt";
int *SM;

typedef struct Customer{
    int id;
    int arrival_time;
    int count;
} Customer;

void get_sem(){
    mutex_id = semget(ftok(MUTEX_PATH, PROJ_ID), 0, 0);
    cook_id = semget(ftok(COOK_PATH, PROJ_ID), 0, 0);
    waiter_id = semget(ftok(WAITER_PATH, PROJ_ID), 0, 0);
    customer_id = semget(ftok(CUSTOMER_PATH, PROJ_ID), 0, 0);
    if(mutex_id == -1 || cook_id == -1 || waiter_id == -1 || customer_id == -1){
        perror("semget");
        exit(1);
    }
}

void get_shm(){
    shmid = shmget(ftok(SHM_PATH, PROJ_ID), 0, 0);
    if(shmid == -1){
        perror("shmget");
        exit(1);
    }
}

void cmain(int id, int count){
    for(;;){
        down(mutex_id, 0);
        if(SM[0] > CLOSING_TIME){
            log_message(SM[0], "Customer %d leaves (late arrival)", id);
            up(mutex_id, 0);
            break;
        }
        if(SM[1] == 0){
            log_message(SM[0], "Customer %d leaves (no empty tables)", id);
            up(mutex_id, 0);
            break;
        }

        SM[1]--;
        int waiter = (id-1)%WAITERS;
        int waiter_fr = 4*(waiter+1);
        int waiter_b = SM[waiter_fr+3];
        SM[waiter_b] = id;
        SM[waiter_b+1] = count;
        SM[waiter_fr+3] = (waiter_b+2); // Enqueue the request
        SM[waiter_fr+1]++;
        up(mutex_id, 0);

        // Signal the waiter to take the order.
        up(waiter_id, waiter);

        // Wait for the waiter to attend
        down(customer_id, id);

        // PLACE ORDER
        down(mutex_id, 0);
        log_message(SM[0], "Customer %d: Order placed to Waiter %d", id, waiter);
        int start_waiting = SM[0];
        up(mutex_id, 0);

        // Wait for food to be served 
        down(customer_id, id);
        down(mutex_id, 0);
        log_message(SM[0], "Customer %d gets food [Waiting Time = %d]", id, SM[0]-start_waiting);
        int curr_time = SM[0];
        up(mutex_id, 0);

        // Eat Food
        int delay = 30;
        msleep(delay);
        curr_time += delay;
        log_message(curr_time, "Customer %d finishes eating and leaves", id);
        down(mutex_id, 0);
        if(SM[0] > curr_time) printf("!!!WARNING: Setting time by CUSTOMER %d failed\n", id);
        else SM[0] = curr_time;
        SM[1]++;
        up(mutex_id, 0);
        break;
    }
    
    exit(0);
}

int main(){
    FILE* fp = fopen(customer_file, "r");
    if(fp == NULL){
        perror("fopen");
        exit(1);
    }

    Customer customers[MAX_CUSTOMERS];
    int idx = 0;
    while(1){
        fscanf(fp, "%d", &customers[idx].id);
        if(customers[idx].id == -1) break;
        fscanf(fp, "%d %d", &customers[idx].arrival_time, &customers[idx].count);
        idx++;
    }
    
    fclose(fp);
    get_sem();
    get_shm();
    signal(SIGINT, cleanup);
    signal(SIGSEGV, cleanup);

    SM = (int*)shmat(shmid, 0, 0);
    int time = 0;

    for(int i=0; i<idx; i++){
        msleep(customers[i].arrival_time - time);
        time = customers[i].arrival_time;
        down(mutex_id, 0);
        log_message(time, "Customer %d arrives (count = %d)", customers[i].id, customers[i].count);
        if(SM[0] > time) printf("!!!WARNING: Setting time by CUSTOMER %d failed\n", customers[i].id);
        else SM[0] = time;
        up(mutex_id, 0);
        pid_t c_pid = fork();
        if(c_pid == -1){
            perror("fork");
            exit(1);
        }
        if(c_pid == 0) cmain(customers[i].id, customers[i].count);
    }

    for(int i=0; i<idx; i++) wait(NULL);
    cleanup(0);
}