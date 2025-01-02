#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>

#define MAX_PERSONS 120
#define SHM_KEY 1234 //klucz pamieci
#define SEM_KEY 5678 //klucz semafor

typedef struct {
    int num_people; //liczba osob na peronie
    int lift_status; // ststus kolejki 1 lub 0
} SharedData; //struct do danych w pamieci wspoldzielonej

int shm_id; //id pamieci wspoldzielonej
SharedData *shm_ptr; //wskaznik na pamiec
int sem_id; //id semaforow

void clean(); 
void sig_handler(int sig);
void start_shmemory(); //tworzenie lub dodawanie do pamieci wspoldzielonej
void start_sem(); //tworzenie i init semaforow

int main() {
    signal(SIGINT, sig_handler); // przechwytywanie ctrl+C do czyszczenia

    start_shmemory();
    start_sem(); //init pamieci i semaforow

    shm_ptr -> num_people = 0;  //ustaw stanu poczatkowego pamieic
    shm_ptr -> lift_status = 1; //kolejka zaczyna jako true

    printf("Stacja uruchomiona. Ctrl+C, aby zakonczyc.\n");

    while (1) { // main loop
        printf("osoby na peronie: %d, status kolejki: %s\n", 
                shm_ptr->num_people, shm_ptr->lift_status ? "dziala" : "zatrzymana");
        sleep(5);
    }
    return 0;
}

void start_shmemory() {
    //tworzenie pamieci
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Nie mozna utworzyc pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }
    //dolaczdenie do pamieci
    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie mozna dolaczyc do pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }
}

void start_sem() {
    // 1 semafor do synchronizacji peronu
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Nie mozna utworzyc semaforow");
        exit(EXIT_FAILURE);
    }
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("nie mozna zainicjalozwoac semafora");
        exit(EXIT_FAILURE);
    }
}

void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("zatrzymano, czysczenie ");
        clean();
        exit(0);
    }
}

void clean() {
    if (shmdt(shm_ptr) == -1) {
        perror("nie mozna odlaczyc pamieci wspoldzielonej");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("nie mozna usunac pamieci wspoldzielonej");
    }
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("nie mozna usunac semaforow");
    }
}