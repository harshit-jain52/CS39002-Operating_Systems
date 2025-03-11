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
#define COOKS 2
#define TABLES 10
#define CLOSING_TIME 240
#define START_TIME_12_HOUR 11
#define SHM_SIZE 2000
#define msleep(x) usleep((x)*100000)

// Semaphore functions
void down(int semid, int i);
void up(int semid, int i);

// Logging functions
void timestamp(int min);
void print_spaces(int spaces);
void log_message(int min, int spaces, const char *format, ...);

// Cleanup signal handler
void cleanup(int signo);