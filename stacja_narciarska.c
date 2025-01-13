#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include "shared.h"

int shm_id; 
int sem_id; 
int msg_id;
SharedData *shm_ptr; 

void clean(); 
void sig_handler(int sig); 
void start_shmemory(); 
void start_sem(); 
int work_time(int start, int stop);
void raport();
void wait_sem(); // operacja obnizenia semafora
void signal_sem(); // operacja podniesieni semafora
void start_msg(); 

int main() {
    pid_t pid_kasjer, pid_pracownik1, pid_pracownik2;

    signal(SIGINT, sig_handler); // Zamkniecie programu
    signal(SIGUSR1, sig_handler); // Zatrzymanie 
    signal(SIGUSR2, sig_handler); // Wznowienie

    // inizjalizacja ipcs
    start_shmemory();
    start_sem();
    start_msg();

    shm_ptr->num_people_lower = 0;  
    shm_ptr->num_chairs = 0;       // Ustawienie wartosci poczatkwoych
    shm_ptr->lift_status = 1;       
    shm_ptr->num_tickets = 0;     
    printf("Stacja uruchomiona. Ctrl+C, aby zakończyć.\n");

    pid_kasjer = fork();
    if (pid_kasjer == 0) {
        execl("./kasjer", "kasjer", NULL);
        perror("Nie udało się uruchomić procesu kasjera");
        exit(EXIT_FAILURE);
    }
    printf("Kasjer PID: %d\n", pid_kasjer);

    pid_pracownik1 = fork();
    if (pid_pracownik1 == 0) {
        execl("./pracownik1", "pracownik1", NULL);
        perror("Nie udało się uruchomić procesu pracownika1");
        exit(EXIT_FAILURE);
    }
    printf("Pracownik1 PID: %d\n", pid_pracownik1);

    pid_pracownik2 = fork();
    if (pid_pracownik2 == 0) {
        execl("./pracownik2", "pracownik2", NULL);
        perror("Nie udało się uruchomić procesu pracownika2");
        exit(EXIT_FAILURE);
    }
    printf("Pracownik2 PID: %d\n", pid_pracownik2);
    sleep(1);
    while (1) {
        // if (!work_time(8, 18)) { // Sprawdzenie godzin pracy
        //     printf("Kolej linowa jest zamknięta.\n");
        //     shm_ptr->lift_status = 0;
        //     sleep(60); // Odczekanie przed kolejną próbą
        //     continue;
        // }

        // sprawdzamy status koleji
        wait_sem();
        printf("Status kolejki: %s, Zajęte krzesełka: %d/%d, Liczba osób na peronie: %d\n",
               shm_ptr->lift_status ? "Działa" : "Zatrzymana",
               shm_ptr->num_chairs, MAX_CHAIRS,
               shm_ptr->num_people_lower);

        // jezeli kolej zatrzymana sprawdzamy warunek i wznawiamy
        if (!shm_ptr->lift_status && shm_ptr->num_chairs < MAX_CHAIRS - 5) {
            printf("Stacja: Wszystkie warunki spełnione, wznawiam kolejkę.\n");
            shm_ptr->lift_status = 1;
            kill(pid_pracownik1, SIGUSR2);
            kill(pid_pracownik2, SIGUSR2);
        }
        signal_sem();

        pid_t pid_narciarz = fork();
        if (pid_narciarz == 0) {
            execl("./narciarz", "narciarz", NULL);
            perror("Nie udało się uruchomić procesu narciarza");
            exit(EXIT_FAILURE);
        }
        printf("Narciarz PID: %d\n", pid_narciarz);
        usleep(1000000);
        //sleep(rand() % 3 + 1); // 
    }
    return 0;
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Stacja: Nie można utworzyć pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie można dołączyć do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }
}

void start_sem() {
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Nie można utworzyć semaforów");
        exit(EXIT_FAILURE);
    }
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("Nie można zainicjalizować semafora");
        exit(EXIT_FAILURE);
    }
}

void start_msg() {
    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("Nie udalo sie utworzyc kolejki komunikatow");
        exit(EXIT_FAILURE);
    }
}

void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("Zatrzymano. Czyszczenie pamięci.\n");
        raport();
        clean();
        exit(0);
    } else if (sig == SIGUSR1) {
        printf("Awaryjne zatrzymanie kolejki!\n");
        shm_ptr->lift_status = 0;
    } else if (sig == SIGUSR2) {
        printf("Wznowienie pracy kolejki przez stację centralną.\n");
        shm_ptr->lift_status = 1;
    }
}

void clean() {
    if (shmdt(shm_ptr) == -1) {
        perror("Nie można odłączyć pamięci współdzielonej");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Nie można usunąć pamięci współdzielonej");
    }
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Nie można usunąć semaforów");
    }
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("Nie mozna usunac kolejki komunikatow");
    }
}

int work_time(int start, int stop) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    int curr_hour = local_time->tm_hour;
    return curr_hour >= start && curr_hour < stop;
}

void raport() {
    wait_sem();
    printf("Raport sprzedaży i użytkowania:\n");
    printf("Liczba osób na peronie: %d\n", shm_ptr->num_people_lower);
    printf("Liczba zajętych krzesełek: %d/%d\n", shm_ptr->num_chairs, MAX_CHAIRS);
    printf("Liczba sprzedanych biletów: %d\n", shm_ptr->num_tickets);

    for (int i = 0; i < shm_ptr->num_tickets; i++) {
        printf("Karnet ID: %d; Typ: %s; Zniżka: %s; Ilosc uzyc: %d; Czas kupna: %s;\n",
               shm_ptr->tickets[i].id, shm_ptr->tickets[i].type,
               shm_ptr->tickets[i].discount ? "tak" : "nie",
               shm_ptr->tickets[i].num_uses,
               ctime(&shm_ptr->tickets[i].bought_time));
    }
    signal_sem();
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
