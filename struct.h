#ifndef STRUCT_H
#define STRUCT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_CHAIRS 20
#define CHAIR_SIZE 3
#define MAX_PLATFORM 100
#define NUM_GATES 4
#define MAX_CHILDREN 2
#define SYNC_CHAIR 3

#define MAX_AGE 80
#define CHILD_AGE_MIN 4
#define CHILD_AGE_MAX 8
#define ADULT_AGE_MIN 12
#define SENIOR_AGE_MIN 65

#define TICKET_TYPE_TK1 1
#define TICKET_TYPE_TK2 2
#define TICKET_TYPE_TK3 3
#define TICKET_TYPE_DAILY 4

#define T1 4
#define T2 6
#define T3 8

#define SEM_GATE1 0
#define SEM_GATE2 1
#define SEM_GATE3 2
#define SEM_GATE4 3
#define SEM_QUEUE 4
#define SEM_RIDE_UP 5
#define TOTAL_SEMS 6

#define SHM_KEY 12345
#define SEM_KEY 54321
#define MSG_KEY 98765

typedef struct {
    long mtype;
    int age;
    int children_count;
    pid_t skier_pid;
    int ticket_type;
    int is_vip;
} TicketRequest;

typedef struct {
    long mtype;
    time_t valid_until;
    double price;
    int ticket_id[MAX_CHILDREN + 1];
} TicketResponse;

typedef struct {
    int people_on_platform;
    int is_running;
} SharedData;

void sem_op(int semid, int sem_num, int op) {
    struct sembuf sb = {sem_num, op, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("Błąd operacji na semaforze");
        exit(1);
    }
}

#endif