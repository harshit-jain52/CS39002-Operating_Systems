#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

/* Linked List Implementation */

typedef struct node
{
    int data;
    struct node *next;
} Node;

Node *makeNode(int data)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    return newNode;
}

// void printLL(Node *head) // O(N)
// {
//     Node *mover = head;
//     while (mover)
//     {
//         printf("%d ", mover->data);
//         mover = mover->next;
//     }
// }

Node *insertTail(Node *head, int el)
{
    if (head == NULL)
        return makeNode(el);

    Node *mover = head;
    while (mover->next)
        mover = mover->next;

    mover->next = makeNode(el);

    return head;
}

Node *copyLL(Node *head, Node *other)
{
    if (other == NULL)
        return NULL;

    head = makeNode(other->data);
    Node *mover1 = head, *mover2 = other;

    while (mover2->next)
    {
        mover1->next = makeNode(mover2->next->data);
        mover1 = mover1->next;
        mover2 = mover2->next;
    }

    return head;
}
/*------------------------------------------------------------------------*/

/* FIFO Queue Implementation */

#define QUEUEMAXLEN 10000

typedef struct
{
    int arr[QUEUEMAXLEN]; // maximum size of the queue is QUEUEMAXLEN-1
    int front;
    int back;
} queue;

queue init()
{
    queue Q;

    Q.front = 0;
    Q.back = QUEUEMAXLEN - 1;

    return Q;
}

bool isEmpty(queue Q)
{
    return (Q.front == (Q.back + 1) % QUEUEMAXLEN);
}

bool isFull(queue Q)
{
    return (Q.front == (Q.back + 2) % QUEUEMAXLEN);
}

int front(queue Q)
{
    if (isEmpty(Q))
    {
        printf("\nfront: Empty Queue\n");
        exit(0);
    }

    return Q.arr[Q.front];
}

queue enqueue(queue Q, int elem)
{
    if (isFull(Q))
    {
        printf("\nenqueue: Full Queue\n");
        exit(0);
    }
    
    Q.back = (Q.back+1) % QUEUEMAXLEN;
    Q.arr[Q.back] = elem;
    return Q;
}

queue dequeue(queue Q)
{
    if (isEmpty(Q))
    {
        printf("\ndequeue: Empty Queue\n");
        exit(0);
    }

    Q.front = (Q.front+1) % QUEUEMAXLEN;
    return Q;
}

/*------------------------------------------------------------------------*/

/* Priority Queue Implementation */

#define HEAPCAPACITY 10000

typedef struct
{
    int timestamp;
    enum
    {
        ARRIVAL,
        IO_COMPLETE,
        TIMEOUT
    } type;
    int pid;
} Event;

int __heap_size = 0; // size of Heap

// Helper Methods for Indices
int Root() { return 1; } // highest priority
int parent(int i) { return i / 2; }
int leftChild(int i) { return 2 * i; }
int rightChild(int i) { return 2 * i + 1; }

// Helper Methods for Node Testing
int hasParent(int i) { return i != Root(); }
int isValidNode(int i) { return i <= __heap_size; }

// Utility Functions
void swap(Event *x, Event *y)
{
    Event tmp = *x;
    *x = *y;
    *y = tmp;
}

// Comparison Functions
bool isGreater(Event a, Event b) //  Priority: a < b
{
    if (a.timestamp != b.timestamp)
        return a.timestamp > b.timestamp;
    if (a.type == TIMEOUT && (b.type == ARRIVAL || b.type == IO_COMPLETE))
        return true;
    if (b.type == TIMEOUT && (a.type == ARRIVAL || a.type == IO_COMPLETE))
        return false;
    return a.pid > b.pid;
}

bool isLess(Event a, Event b) // Priority: a > b
{
    if (a.timestamp != b.timestamp)
        return a.timestamp < b.timestamp;
    if (b.type == TIMEOUT && (a.type == ARRIVAL || a.type == IO_COMPLETE))
        return true;
    if (a.type == TIMEOUT && (b.type == ARRIVAL || b.type == IO_COMPLETE))
        return false;
    return a.pid < b.pid;
}

Event top(Event H[]) // O(1)
{
    if (__heap_size == 0)
    {
        printf("\nfront: Empty Priority Queue\n");
        exit(0);
    }

    return H[Root()];
}

void shiftUp(Event H[], int idx)
{
    while (hasParent(idx) && isGreater(H[parent(idx)], H[idx]))
    {
        swap(&H[parent(idx)], &H[idx]);
        idx = parent(idx);
    }
}

void shiftDown(Event H[], int idx)
{
    while (isValidNode(leftChild(idx)))
    {
        int child = leftChild(idx);

        if (isValidNode(rightChild(idx)) && isLess(H[rightChild(idx)], H[leftChild(idx)]))
            child = rightChild(idx);

        if (isGreater(H[idx], H[child]))
            swap(&H[idx], &H[child]);
        else
            break;

        idx = child;
    }
}

void push(Event H[], Event newNum) // O(logN)
{
    if (__heap_size == HEAPCAPACITY)
    {
        printf("\npush: Full Priority Queue\n");
        exit(0);
    }

    H[++__heap_size] = newNum; // insert at the last
    shiftUp(H, __heap_size);
}

void pop(Event H[]) // O(logN)
{
    if (__heap_size == 0)
    {
        printf("\npop: Empty Priority Queue\n");
        exit(0);
    }

    H[Root()] = H[__heap_size--]; // move last element at root
    shiftDown(H, Root());
}

/*------------------------------------------------------------------------*/

// Process Control Block
typedef struct
{
    int pid;        // Process ID (0-indexed)
    int arv_time;   // Arrival Time
    int num_bursts; // Number of CPU and IO Bursts
    Node *bursts;   // CPU and IO Bursts
    Node *curr;     // Current Burst
    int wait_time;
    int turnaround_time;
    int last_insert;
    int run_time;
} PCB;

int min(int x, int y)
{
    return (x < y) ? x : y;
}

const char *PROC_TXT_FILE = "proc.txt";
int num_procs;
const int INF = 1e9;
PCB *procs = NULL;

// Round Robin Scheduling Algorithm
void round_robin(int q)
{
    Event EQ[HEAPCAPACITY + 1];
    __heap_size = 0;

    for (int i = 0; i < num_procs; i++)
    {
        Event arrv = {
            .pid = procs[i].pid,
            .timestamp = procs[i].arv_time,
            .type = ARRIVAL};
        push(EQ, arrv);

        procs[i].curr = copyLL(procs[i].curr, procs[i].bursts);
        procs[i].wait_time = 0;
    }

    queue RQ = init();
    bool cpu_busy = false;
    int curr_time = 0;
    int cpu_idle_time = 0, last_cpu_idle = 0;
    double avg_wait_time = 0;
#ifdef VERBOSE
    printf("0\t: Starting\n");
#endif
    while (__heap_size)
    {
        Event e = top(EQ);
        pop(EQ);
        curr_time = e.timestamp;
        if (e.type == ARRIVAL)
        {
            RQ = enqueue(RQ, e.pid);
            procs[e.pid].last_insert = curr_time;
#ifdef VERBOSE
            printf("%d\t: Process %d joins ready queue upon arrival\n", curr_time, e.pid + 1);
#endif
        }
        else if (e.type == IO_COMPLETE)
        {
            RQ = enqueue(RQ, e.pid);
            procs[e.pid].last_insert = curr_time;
#ifdef VERBOSE
            printf("%d\t: Process %d joins ready queue after IO completion\n", curr_time, e.pid + 1);
#endif
        }
        else
        {
            cpu_busy = false;
            last_cpu_idle = curr_time;
            int id = e.pid;
            if (procs[id].curr->data == 0)
            {
                // CPU burst ends
                if (procs[id].curr->next->data == -1)
                {
                    procs[id].turnaround_time = curr_time - procs[id].arv_time;
                    procs[id].run_time = (int)round(procs[id].turnaround_time * 100.0 / (procs[id].turnaround_time - procs[id].wait_time));
                    avg_wait_time += procs[id].wait_time;
                    printf("%d\t: Process %d exits. Turnaround time = %d (%d%%), Wait time = %d\n", curr_time, id + 1, procs[id].turnaround_time, procs[id].run_time, procs[id].wait_time); // Process exits
                }
                else
                {
                    // Next IO burst
                    Event tmp = {
                        .pid = id,
                        .timestamp = curr_time + procs[id].curr->next->data,
                        .type = IO_COMPLETE};
                    push(EQ, tmp);
                    procs[id].curr = procs[id].curr->next->next;
                }
            }
            else
            {
                // Preemption
                RQ = enqueue(RQ, e.pid);
                procs[e.pid].last_insert = curr_time;
#ifdef VERBOSE
                printf("%d\t: Process %d joins ready queue after timeout\n", curr_time, e.pid + 1);
#endif
            }
        }

        if (!cpu_busy && !isEmpty(RQ))
        {
            int fid = front(RQ);
            RQ = dequeue(RQ);
            procs[fid].wait_time += curr_time - procs[fid].last_insert;
            int mn = min(procs[fid].curr->data, q);
            procs[fid].curr->data -= mn;
            Event tmp = {
                .pid = fid,
                .type = TIMEOUT,
                .timestamp = curr_time + mn};
            push(EQ, tmp);
#ifdef VERBOSE
            printf("%d\t: Process %d is scheduled to run for time %d\n", curr_time, fid + 1, mn);
#endif
            cpu_busy = true;
            cpu_idle_time += curr_time - last_cpu_idle;
        }
#ifdef VERBOSE
        if (!cpu_busy)
            printf("%d\t: CPU goes idle\n", curr_time);
#endif
    }

    int total_turnaround_time = curr_time;
    avg_wait_time /= num_procs;
    double cpu_util = (total_turnaround_time - cpu_idle_time) * 100.0 / curr_time;

    printf("\n");
    printf("Average wait time = %.2lf\n", avg_wait_time);
    printf("Total turnaround time = %d\n", total_turnaround_time);
    printf("CPU idle time = %d\n", cpu_idle_time);
    printf("CPU utilization = %.2lf%%\n", cpu_util);
}

int main()
{
    FILE *fp = fopen(PROC_TXT_FILE, "r");
    if (fp == NULL)
    {
        printf("Could not open file: %s", PROC_TXT_FILE);
        exit(1);
    }

    fscanf(fp, "%d", &num_procs);
    procs = (PCB *)malloc(num_procs * sizeof(PCB));
    for (int i = 0; i < num_procs; i++)
    {
        fscanf(fp, "%d", &procs[i].pid);
        fscanf(fp, "%d", &procs[i].arv_time);
        procs[i].pid--;
        procs[i].bursts = NULL;
        procs[i].num_bursts = 0;

        while (1)
        {
            int x;
            fscanf(fp, "%d", &x);
            procs[i].num_bursts++;
            procs[i].bursts = insertTail(procs[i].bursts, x);
            if (x == -1)
                break;
        }

        procs[i].curr = procs[i].bursts;
        procs[i].wait_time = 0;
    }
    fclose(fp);

    // for(int i=0;i<num_procs;i++){
    //     printf("%d\t%d\t%d\t",procs[i].pid,procs[i].arv_time,procs[i].num_bursts);
    //     printLL(procs[i].bursts);
    //     printf("\t%d",procs[i].curr->data);
    //     printf("\n");
    // }

    printf("\n**** FCFS Scheduling ****\n");
    round_robin(INF);

    printf("\n**** RR Scheduling with q = 10 ****\n");
    round_robin(10);

    printf("\n**** RR Scheduling with q = 5 ****\n");
    round_robin(5);
}