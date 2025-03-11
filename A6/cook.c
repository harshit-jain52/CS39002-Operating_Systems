#include <barfoobar.h>
int mutex_id, cook_id, waiter_id, customer_id;
int shmid;

void init_sem(){
    mutex_id = semget(ftok(MUTEX_PATH, PROJ_ID), 1, 0777|IPC_CREAT|IPC_EXCL);
    cook_id = semget(ftok(COOK_PATH, PROJ_ID), 1, 0777|IPC_CREAT|IPC_EXCL);
    waiter_id = semget(ftok(WAITER_PATH, PROJ_ID), WAITERS, 0777|IPC_CREAT|IPC_EXCL);
    customer_id = semget(ftok(CUSTOMER_PATH, PROJ_ID), MAX_CUSTOMERS, 0777|IPC_CREAT|IPC_EXCL);

    if(mutex_id == -1 || cook_id == -1 || waiter_id == -1 || customer_id == -1){
        perror("semget");
        exit(1);
    }

    semctl(mutex_id, 0, SETVAL, 1);  
    semctl(cook_id, 0, SETVAL, 0);   
    for(int i = 0; i < WAITERS; i++) semctl(waiter_id, i, SETVAL, 0);
    for(int i = 0; i < MAX_CUSTOMERS; i++) semctl(customer_id, i, SETVAL, 0);
}

void init_shm(){
    shmid = shmget(ftok(SHM_PATH, PROJ_ID), SHM_SIZE*sizeof(int), 0777|IPC_CREAT|IPC_EXCL);
    
    if(shmid == -1){
        perror("shmget");
        exit(1);
    }
    int* SM = (int*)shmat(shmid, 0, 0);
    if(SM == (void*)-1){
        perror("shmat");
        exit(1);
    }
    
    /*
    First 100 cells Global int variables (you don’t have to use all the cells)
    Cells 100 – 299 For waiter U
    Cells 300 – 499 For waiter V
    Cells 500 – 699 For waiter W
    Cells 700 – 899 For waiter X
    Cells 900 – 1099 For waiter Y
    Cells 1100 – 2000 For cooking queue
    */
    
    // Global int variables
    SM[0] = 0; // time
    SM[1] = TABLES; // no. of empty tables
    SM[2] = 0; // waiter no. to serve the next customer
    SM[3] = 0; // no. of orders pending for the cooks
    
    SM[4] = 0; // FR for Waiter U
    SM[5] = 0; // PO for Waiter U
    SM[6] = 100; // F for Waiter U
    SM[7] = 100; // B for Waiter U

    SM[8] = 0; // FR for Waiter V
    SM[9] = 0; // PO for Waiter V
    SM[10] = 300; // F for Waiter V
    SM[11] = 300; // B for Waiter V

    SM[12] = 0; // FR for Waiter W
    SM[13] = 0; // PO for Waiter W
    SM[14] = 500; // F for Waiter W
    SM[15] = 500; // B for Waiter W
    
    SM[16] = 0; // FR for Waiter X
    SM[17] = 0; // PO for Waiter X
    SM[18] = 700; // F for Waiter X
    SM[19] = 700; // B for Waiter X
    
    SM[20] = 0; // FR for Waiter Y
    SM[21] = 0; // PO for Waiter Y
    SM[22] = 900; // F for Waiter Y
    SM[23] = 900; // B for Waiter Y
    
    SM[24] = 1100; // F for Cooking Queue
    SM[25] = 1100; // B for Cooking Queue
    
    SM[26] = COOKS; // no. of cooks available
    SM[30] = 0; // no. of unprepared orders for Waiter U
    SM[31] = 0; // no. of unprepared orders for Waiter V
    SM[32] = 0; // no. of unprepared orders for Waiter W
    SM[33] = 0; // no. of unprepared orders for Waiter X
    SM[34] = 0; // no. of unprepared orders for Waiter Y

    // For waiter U
    for(int i=100; i<=299; i++) SM[i] = 0;

    // For waiter V
    for(int i=300; i<=499; i++) SM[i] = 0;

    // For waiter W
    for(int i=500; i<=699; i++) SM[i] = 0; 

    // For waiter X
    for(int i=700; i<=899; i++) SM[i] = 0; 

    // For waiter Y
    for(int i=900; i<=1099; i++) SM[i] = 0; 

    // For cooking queue
    for(int i=1100; i<SHM_SIZE; i++) SM[i] = 0; 

    shmdt(SM);
}


void cmain(char cook){
    int *SM = (int*)shmat(shmid, 0, 0);
    if(SM == (void*)-1){
        perror("shmat");
        exit(1);
    }

    log_message(0, cook-'C', "Cook %c is ready", cook);
    for(;;){
        // Wait until woken up by a waiter submitting a cooking request
        down(cook_id, 0);

        // Check for End of Session
        bool leave = false;
        down(mutex_id, 0);
        if(SM[26]!=COOKS){
            log_message(SM[0], cook-'C', "Cook %c: Leaving", cook);
            leave = true;
            SM[26]--;
            if(SM[26] == 0){
                for(int i=0; i<WAITERS; i++) up(waiter_id, i);
            }
        }
        
        up(mutex_id, 0);
        if(leave) break;

        // Read a cooking request
        down(mutex_id, 0);
        int top = SM[24];
        int waiter_num = SM[top];
        int cust_id = SM[top+1];
        int cust_ct = SM[top+2];
        SM[24] = (top+3); // Dequeue the request
        int curr_time = SM[0];
        up(mutex_id, 0);

        // Prepare the food
        log_message(curr_time, cook-'C', "Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)", cook, 'U'+waiter_num, cust_id, cust_ct);
        int delay = 5*cust_ct;
        msleep(delay);
        curr_time += delay;
        log_message(curr_time, cook-'C', "Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)", cook, 'U'+waiter_num, cust_id, cust_ct);

        // Notify the waiter that the food is ready
        down(mutex_id, 0);
        int fr = 4*(waiter_num+1);
        SM[fr] = cust_id;
        if(SM[0] > curr_time) printf("!!!WARNING: Setting time by COOK %c failed\n", cook);
        else SM[0] = curr_time;
        up(waiter_id, waiter_num);
        up(mutex_id, 0);

        // Check for End of Session
        leave = false;
        down(mutex_id, 0);
        if(SM[0]>CLOSING_TIME && SM[24]==SM[25]){
            log_message(SM[0], cook-'C', "Cook %c: Leaving", cook);
            leave = true;
            SM[26]--;
            if(SM[26] == 0){
                for(int i=0; i<WAITERS; i++) up(waiter_id, i);
            }
            else{
                for(int i=0; i<COOKS-1; i++) up(cook_id, 0);
            }
        }
        up(mutex_id, 0);
        if(leave) break;
    }

    exit(0);
}

int main(){
    init_sem();
    init_shm();
    signal(SIGINT, cleanup);
    signal(SIGSEGV, cleanup);
    
    for(int i=0; i<COOKS; i++){
        pid_t c_pid = fork();
        if(c_pid == -1){
            perror("fork");
            exit(1);
        }
        if(c_pid == 0) cmain('C'+i);
    }

    for(int i=0; i<COOKS; i++) wait(NULL);
}