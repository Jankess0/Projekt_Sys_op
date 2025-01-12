#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared.h"

int shm_id;
int sem_id;
SharedData *shm_ptr;

void sell_ticket(int years, const char *type);
void raport();
void start_shmemory();
void wait_sem();
void signal_sem();

int main() {
    srand(time(NULL));
    start_shmemory();

    wait_sem();
    if (shm_ptr->num_tickets == 0) {
        shm_ptr->num_tickets = 0;
    }
    signal_sem();

    printf("Kasjer uruchomiony\n");

    while (1) {
        int years = rand() % 80;
        const char *type;
        int num = rand() % 4 + 1;
        switch (num) {
            case 1:
                type = "czasowy2h";
                break;
            case 2:
                type = "czasowy4h";
                break;
            case 3:
                type = "czasowy6h";
                break;
            case 4:
                type = "dzienny";
                break;
        }

        sell_ticket(years, type);

        // wait_sem();
        // if (shm_ptr->num_tickets % 10 == 0 && shm_ptr->num_tickets > 0) {
        //     signal_sem(); // Odblokowanie przed wywołaniem raportu
        //     raport();
        // } else {
        //     signal_sem();
        // }
        //usleep(50000);
        // sleep(rand() % 3 + 1);
    }
    return 0;
}

void sell_ticket(int years, const char *type) {
    wait_sem();

    if (shm_ptr->num_tickets >= MAX_TICKETS) {
        printf("Osiągnięto limit sprzedaży biletów\n");
        signal_sem();
        sleep(60);
        return;
    }

    Ticket *new_ticket = &shm_ptr->tickets[shm_ptr->num_tickets];
    new_ticket->id = shm_ptr->num_tickets + 1;
    strncpy(new_ticket->type, type, sizeof(new_ticket->type) - 1);
    new_ticket->type[sizeof(new_ticket->type) - 1] = '\0';
    new_ticket->discount = (years < 12 || years > 65) ? 1 : 0;
    new_ticket->bought_time = time(NULL);
    new_ticket->age = years;
    new_ticket->num_uses = 0;

    shm_ptr->num_tickets++;

    // printf("Sprzedano karnet ID: %d, Typ: %s, Wiek: %d, Zniżka: %s, Czas kupna: %s\n",
    //        new_ticket->id, new_ticket->type, new_ticket->age,
    //        new_ticket->discount ? "tak" : "nie",
    //        ctime(&new_ticket->bought_time));

    signal_sem();
}

void raport() {
    wait_sem();
    printf("Raport sprzedaży:\n");
    printf("Liczba sprzedanych biletów: %d\n", shm_ptr->num_tickets);

    for (int i = 0; i < shm_ptr->num_tickets; i++) {
        printf("Karnet ID: %d; Typ: %s; Wiek: %d; Zniżka: %s; Czas kupna: %s\n",
               shm_ptr->tickets[i].id, shm_ptr->tickets[i].type, shm_ptr->tickets[i].age,
               shm_ptr->tickets[i].discount ? "tak" : "nie",
               ctime(&shm_ptr->tickets[i].bought_time));
    }
    signal_sem();
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Kasjer: Nie można utworzyć pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie można dołączyć do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Nie można utworzyć semaforów");
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
