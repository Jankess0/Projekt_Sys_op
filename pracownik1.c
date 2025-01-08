#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shared.h"

int shm_id;
SharedData *shm_ptr;

void sig_handler(int sig);
void start_shmemory();

int main() {
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    start_shmemory();
    printf("Pracownik1 uruchomiony, monitoruje dolna stacje.\n");

    while (1) {
        pause();
    }
    return 0;
}

void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("Pracownik1: Zatrzymuje kolej!\n");
        shm_ptr->lift_status = 0;
    } else if (sig == SIGUSR2) {
        printf("Pracownik1: Wznawia prace koleji!\n");
        shm_ptr->lift_status = 1;
    }
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Nie mozna utworzyc pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie mozna dolaczyc do pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }
}