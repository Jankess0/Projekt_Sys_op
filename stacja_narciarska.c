#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <time.h>
#include "shared.h"

#define SHM_KEY 1234 //klucz pamieci
#define SEM_KEY 5678 //klucz semafor


int shm_id; //id pamieci wspoldzielonej
SharedData *shm_ptr; //wskaznik na pamiec
int sem_id; //id semaforow

void clean(); 
void sig_handler(int sig);
void start_shmemory(); //tworzenie lub dodawanie do pamieci wspoldzielonej
void start_sem(); //tworzenie i init semaforow
int work_time(int start, int stop);
void raport();

int main() {
    pid_t pid_kasjer, pid_pracownik1, pid_pracownik2;

    signal(SIGINT, sig_handler); // przechwytywanie ctrl+C do czyszczenia
  

    start_shmemory();
    start_sem(); //init pamieci i semaforow

    shm_ptr -> num_people = 0;  //ustaw stanu poczatkowego pamieic
    shm_ptr -> lift_status = 1; //kolejka zaczyna jako true
    shm_ptr -> num_tickets = 0; //ustawia liczbe biletow na 0
    printf("Stacja uruchomiona. Ctrl+C, aby zakonczyc.\n");

    pid_kasjer = fork();
    if (pid_kasjer == 0) {
        execl("./kasjer", "kasjer", NULL);
        perror("nie udalo sie uruchomic proces kasjer");
        exit(EXIT_FAILURE);
    }

    // pid_pracownik1 = fork();
    // if (pid_pracownik1 == 0) {
    //     execl("./pracwonik1", "pracownik1", NULL);
    //     perror("nie udalo sie uruchomic procesu pracownik1");
    //     exit(EXIT_FAILURE);
    // }

    // pid_pracownik2 = fork();
    // if (pid_pracownik2 == 0) {
    //     execl("./pracownik2", "pracownik2", NULL);
    //     perror("nie udalo sie uruchomic procesu pracownik2");
    //     exit(EXIT_FAILURE);
    // }

    while (1) { // main loop
        // if (!work_time(8, 18)) {//sprawdznie godzin
        //     printf("kolej zamknieta\n");
        //     sleep(60);
        //     continue;
        // }
        // while (1) {
        //     pid_t pid_narciarz = fork();
        //     if (pid_narciarz == 0) {
        //         execl("./narciarz", "narciarz", NULL);
        //         perror("Nie udało się uruchomić procesu narciarza");
        //         exit(EXIT_FAILURE);
        //     }
        //     sleep(rand() % 3 + 1);
        // }
        // if (shm_ptr -> num_people > MAX_PERSONS) {//sprawdzenie ilosci osob
        //     printf("przepelnainie kolejki %d osob\n", shm_ptr->num_people);
        //     shm_ptr -> num_people = MAX_PERSONS;
        // }
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
        raport();
        clean();
        exit(0);
    // } else if (sig == SIGUSR1) {
    //     printf("awaryjne zatrzymanie kolejki\n");
    //     shm_ptr -> lift_status = 0;
    // } else if (sig == SIGUSR2) {
    //     printf("wznowienie kolejki\n");
    //     shm_ptr -> lift_status = 1; 
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

int work_time(int start, int stop) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    int curr_hour = local_time -> tm_hour;
    return curr_hour >= start && curr_hour < stop;
}

void raport() {
    printf("raport sprzedarzy\n");
    for (int i = 0; i < shm_ptr -> num_tickets; i++) {
        printf("Karnet ID: %d; typ: %s; znizka: %s; czas kupna: %s;\n",
        shm_ptr -> tickets[i].id, shm_ptr -> tickets[i].type,
        shm_ptr -> tickets[i].discount ? "tak" : "nie",
        ctime(&shm_ptr -> tickets[i].bought_time));
    }
}