#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "shared.h"

int shm_id;
int sem_id;
SharedData *shm_ptr;

void start_shmemory();
void wait_sem();
void signal_sem();
void* child_behavior(void* arg);
int entry_queue(FifoQueue *queue, FifoEntry entry);
void handle_signal(int sig);

typedef struct {
    int ticket_id;
    int is_child;
} TicketData;

int main() {
    srand(time(NULL) + getpid());
    start_shmemory();
    signal(SIGUSR1, handle_signal);
    wait_sem();
    int ticket_index = -1;
    int attempts = 0;
    int temp;

    while (attempts < 10) { // Maksymalnie 10 prób na wybór biletu
        if (shm_ptr->num_tickets <= 0) {
            printf("Narciarz %d: Brak dostępnych biletów.\n", getpid());
            signal_sem();
            return 0;
        }

        ticket_index = rand() % shm_ptr->num_tickets;

        // Sprawdzamy, czy bilet jest dostępny i spełnia wymagania
        if (!shm_ptr->tickets[ticket_index].is_in_use && shm_ptr->tickets[ticket_index].age >= 8) {
            shm_ptr->tickets[ticket_index].is_in_use = 1; // Oznacz bilet jako zajęty
            shm_ptr->tickets[ticket_index].num_uses++;
            printf("Narciarz %d: Wybrano bilet ID %d, wiek: %d\n", getpid(),
                   shm_ptr->tickets[ticket_index].id, shm_ptr->tickets[ticket_index].age);
            break;
        }

        attempts++;
    }

    if (attempts >= 10) {
        printf("Narciarz %d: Nie znaleziono odpowiedniego biletu po 10 próbach. Rezygnuję.\n", getpid());
        signal_sem();
        return 0;
    }
    int children = rand() % 3; // Dorosły może zabrać maksymalnie 2 dzieci
    if (children == 3) children = 2;

    temp = children;

    for (int i = 0; i < temp; i++) {
        int child_ticket_index = -1;
        attempts = 0;

        while (attempts < 30) {
            if (shm_ptr->num_tickets > 0) {
                child_ticket_index = rand() % shm_ptr->num_tickets;

                if (!shm_ptr->tickets[child_ticket_index].is_in_use &&
                    shm_ptr->tickets[child_ticket_index].age < 8) {
                    shm_ptr->tickets[child_ticket_index].is_in_use = 1; // Oznacz bilet jako zajęty
                    shm_ptr->tickets[child_ticket_index].num_uses++;
                    printf("Narciarz %d: Dziecko %d otrzymało bilet ID %d, wiek: %d\n",
                        getpid(), i + 1, shm_ptr->tickets[child_ticket_index].id,
                        shm_ptr->tickets[child_ticket_index].age);
                    break;
                }
            }

            attempts++;
            if (attempts >= 30) {
                printf("Narciarz %d: Dziecko %d nie otrzymało biletu po 10 próbach. Rezygnuję.\n", getpid(), i + 1);
                children--;
            }

            //sleep(1);
        }
    }

    shm_ptr->num_people_lower += 1 + children;

    FifoEntry entry;
    entry.ticket_id = shm_ptr->tickets[ticket_index].id;
    entry.num_children = children;

    if (entry_queue(&shm_ptr->fifo_queue, entry) == 0) {
        printf("Narciarz %d: Dodano do kolejki z %d dziecmi.\n", entry.ticket_id, entry.num_children);
    } else {
        printf("Narciarz %d: Kolejka jest pelna!\n", entry.ticket_id);
    }

    signal_sem();
    
    printf("Narciarz %d: Czekam na sygnał od pracownika...\n", getpid());
    pause();
    
    return 0;
}
void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        printf("Narciarz %d: Otrzymałem sygnał do rozpoczęcia zjazdu.\n", getpid());
        sleep(rand() % 3 + 1); // Symulacja zjazdu
        printf("Narciarz %d: Zakończyłem zjazd. Kończę proces.\n", getpid());
        exit(0);
    }
}
int entry_queue(FifoQueue *queue, FifoEntry entry) {
    if ((queue->rear+1) % MAX_QUEUE_SIZE == queue -> front) return -1; //kolejka pelna
    queue->queue[queue->rear] = entry;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    return 0;
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Narcairz: Nie można utworzyć pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie można dołączyć do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Nie można uzyskać semaforów");
        exit(EXIT_FAILURE);
    }
}

void wait_sem() {
    struct sembuf sem_op = {0, -1, 0};
    if (semop(sem_id, &sem_op, 1) == -1) {
        perror("Nie można zablokować semafora");
        exit(EXIT_FAILURE);
    }
}

void signal_sem() {
    struct sembuf sem_op = {0, 1, 0};
    if (semop(sem_id, &sem_op, 1) == -1) {
        perror("Nie można odblokować semafora");
        exit(EXIT_FAILURE);
    }
}
