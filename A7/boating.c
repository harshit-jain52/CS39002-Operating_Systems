#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define MMAX 10
#define NMAX 100
#define MMIN 5
#define NMIN 20
#define msleep(x) usleep((x)*100000)

typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;
#define SEMAPHORE_INITIALIZER {0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER}

int m, n;
semaphore boat, rider;
pthread_mutex_t bmtx;
pthread_barrier_t EOS;
bool BA[MMAX];
pthread_barrier_t BB[MMAX];
int BC[MMAX];
time_t BT[MMAX];

int rand_range(int lo, int hi){
    return lo + rand() % (hi - lo + 1);
}

// Wait
void P(semaphore *s){
    pthread_mutex_lock(&(s->mtx));
    s->value--;
    if(s->value < 0) pthread_cond_wait(&(s->cv), &(s->mtx));
    pthread_mutex_unlock(&(s->mtx));
}

// Signal
void V(semaphore *s){
    pthread_mutex_lock(&(s->mtx));
    s->value++;
    if(s->value <= 0) pthread_cond_signal(&(s->cv));
    pthread_mutex_unlock(&(s->mtx));
}

void *Boat(void *targ){
    int boat_id = *(int *)targ;
    printf("Boat\t%d\tReady\n",boat_id+1);
    pthread_mutex_lock(&bmtx);
    pthread_barrier_init(&BB[boat_id], NULL, 2);
    pthread_mutex_unlock(&bmtx);
    while(1){
        P(&boat);
        
        pthread_mutex_lock(&bmtx);
        BA[boat_id] = true;
        BC[boat_id] = -1;
        pthread_barrier_t* bar = &BB[boat_id];
        pthread_mutex_unlock(&bmtx);

        V(&rider);
        
        pthread_barrier_wait(bar);
        
        pthread_mutex_lock(&bmtx);
        time_t rtime = BT[boat_id];
        BA[boat_id] = false;
        int vis_id = BC[boat_id];
        pthread_mutex_unlock(&bmtx);

        printf("Boat\t%d\tStart of ride for visiter %3d\n",boat_id+1,vis_id+1);
        msleep(rtime);
        printf("Boat\t%d\tEnd of ride for visiter %3d (ride time = %3ld)\n",boat_id+1,vis_id+1,rtime);

        pthread_mutex_lock(&bmtx);
        n--;
        if(n == 0){
            pthread_mutex_unlock(&bmtx);
            pthread_barrier_wait(&EOS);
            break;
        }
        pthread_mutex_unlock(&bmtx);   
    }
    pthread_exit(NULL);
}

void *Visitor(void *targ){
    int vis_id = *(int *)targ;
    time_t vtime, rtime;
    vtime = rand_range(30, 120);
    rtime = rand_range(15, 60);

    printf("Visitor\t%d\tStarts sightseeing for %3ld minutes\n",vis_id+1,vtime);
    msleep(vtime);
    V(&boat);
    P(&rider);
    printf("Visitor\t%d\tReady to ride a boat (ride time = %3ld)\n",vis_id+1,rtime);

    int boat_id = -1;
    pthread_barrier_t* bar;
    for(int i = 0; i < m; i++){
        pthread_mutex_lock(&bmtx);
        if(BA[i] && BC[i] == -1){
            boat_id = i;
            BC[boat_id] = vis_id;
            BT[boat_id] = rtime;
            bar = &BB[boat_id];
            pthread_mutex_unlock(&bmtx);
            break;
        }
        pthread_mutex_unlock(&bmtx);
    }

    printf("Visitor\t%d\tFinds boat %2d\n", vis_id+1, boat_id+1);
    pthread_barrier_wait(bar);
    msleep(rtime);

    printf("Visitor\t%d\tLeaving\n", vis_id+1);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        printf("Usage: %s [m] [n]\n", argv[0]);
        return 1;
    }
    
    m = atoi(argv[1]); // No. of boats
    n = atoi(argv[2]); // No. of visitors

    if(m < MMIN || m > MMAX || n < NMIN || n > NMAX){
        printf("Range of m: %d-%d\n", MMIN, MMAX);
        printf("Range of n: %d-%d\n", NMIN, NMAX);
        return 1;
    }

    printf("--- Welcome to Footanical Barden: %d boats, %d visitors ---\n", m, n);

    srand(time(NULL));

    boat = (semaphore)SEMAPHORE_INITIALIZER;
    rider = (semaphore)SEMAPHORE_INITIALIZER;
    bmtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    pthread_barrier_init(&EOS, NULL, 2);

    pthread_t boat_thr[MMAX];
    pthread_t vis_thr[NMAX];
    for(int i=0; i<m; i++){
        int* id = (int*)malloc(sizeof(int));
        *id = i;
        pthread_create(&boat_thr[i], NULL, Boat, id);
    }
    for(int i=0; i<n; i++){
        int* id = (int*)malloc(sizeof(int));
        *id = i;
        pthread_create(&vis_thr[i], NULL, Visitor, id);
    }

    pthread_barrier_wait(&EOS);

    for(int i=0; i<m; i++) pthread_cancel(boat_thr[i]);
    for(int i=0; i<n; i++) pthread_cancel(vis_thr[i]);
    for(int i=0; i<m; i++) pthread_barrier_destroy(&BB[i]);
    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&(boat.mtx));
    pthread_mutex_destroy(&(rider.mtx));
    pthread_cond_destroy(&(boat.cv));
    pthread_cond_destroy(&(rider.cv));

    pthread_barrier_destroy(&EOS);

    printf("--- All visitors have left ---\n");
    return 0;
}