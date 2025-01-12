#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared.h"

int shm_id;
int sem_id;
SharedData *shm_ptr;

void start_shmemory();
void wait_sem();
void signal_sem();
void monitor_peron_gorny();

int main() {
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    start_shmemory();

    printf("Pracownik2 uruchomiony, monitoruje górną stację.\n");
    monitor_peron_gorny();

    return 0;
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Pracownik2: Nie można utworzyć pamięci współdzielonej");
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

void monitor_peron_gorny() {
    while (1) {
        wait_sem();
        if (shm_ptr->num_people_upper > MAX_PERSONS || shm_ptr->num_chairs >= MAX_CHAIRS) {
            printf("Pracownik2: Przepelnienie peronu górnego lub brak dostępnych krzesełek! Zatrzymuję kolejkę.\n");
            shm_ptr->lift_status = 0;
            kill(getppid(), SIGUSR1); 
        } else if (shm_ptr->lift_status == 0 && shm_ptr->num_chairs < MAX_CHAIRS) {
            printf("Pracownik2: Warunki spełnione. Wznowienie pracy kolejki.\n");
            shm_ptr->lift_status = 1;
            kill(getppid(), SIGUSR2);
        }
        signal_sem();
        sleep(1);
    }
}
