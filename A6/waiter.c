#include <barfoobar.h>
int mutex_id, cook_id, waiter_id, customer_id;
int shmid;

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

void wmain(char waiter){
    int *SM = (int*)shmat(shmid, 0, 0);
    if(SM == (void*)-1){
        perror("shmat");
        exit(1);
    }

    log_message(0, "Waiter %c is ready", waiter);
    for(;;){
        // Wait until woken up by a cook or a new customer
        down(waiter_id, waiter-'U');

        down(mutex_id, 0);
        int fr = 4*(waiter-'U'+1);
        int po = fr+1;
        if(SM[fr] > 0){
            // Serve the food
            int cust_id = SM[fr];
            SM[fr] = 0;
            log_message(SM[0], "Waiter %c: Serving food to Customer %d", waiter, cust_id);
            up(customer_id, cust_id-1);

            if(SM[0] > CLOSING_TIME && SM[fr+2]==SM[fr+3]){
                log_message(SM[0], "Waiter %c leaving (no more customers to serve)", waiter);
                up(mutex_id, 0);
                break;
            }
        }
        else if(SM[po] > 0){
            // Take the order
            int f = SM[fr+2];
            int cust_id = SM[f];
            int cust_ct = SM[f+1];
            SM[fr+2] = f+2; // Dequeue the request
            SM[po]--;
            int curr_time = SM[0];
            up(mutex_id, 0);
            int delay = 1;
            msleep(delay);
            curr_time += delay;
            up(customer_id, cust_id-1);

            // Place the order
            log_message(curr_time, "Waiter %c: Placing order for Customer %d (Count %d)", waiter, cust_id, cust_ct);
            down(mutex_id, 0);
            if(SM[0] > curr_time) printf("!!!WARNING: Setting time by WAITER %c failed\n", waiter);
            else SM[0] = curr_time;
            int cook_b = SM[25];
            SM[cook_b] = waiter-'U';
            SM[cook_b+1] = cust_id;
            SM[cook_b+2] = cust_ct;
            SM[25] = (cook_b+3); // Enqueue the request
            up(cook_id, 0);
        }
        else{
            log_message(SM[0], "Waiter %c leaving (no more customers to serve)", waiter);
            up(mutex_id, 0);
            break;
        }
        up(mutex_id, 0);
    }

    exit(0);
}

int main(){
    get_sem();
    get_shm();
    signal(SIGINT, cleanup);
    signal(SIGSEGV, cleanup);

    for(int i=0; i<WAITERS; i++){
        pid_t w_pid = fork();
        if(w_pid == -1){
            perror("fork");
            exit(1);
        }
        if(w_pid == 0) wmain('U'+i);
    }

    for(int i=0; i<WAITERS; i++) wait(NULL);
}