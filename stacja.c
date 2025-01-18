#include "struct.h"

int msg_id;
int sem_id;
int shm_id;
SharedData *shm_ptr;

void start_msg();
void start_sem();
void start_shm();
void clean();
void sig_handler(int sig);

int main() {
    srand(time(NULL));
    signal(SIGINT, sig_handler);
    start_msg();
    start_sem();
    start_shm();
    pid_t pid_kasjer, pid_narciarz, pid_pracownik;

    shm_ptr->people_on_platform = 0;
    shm_ptr->is_running = 1;

      pid_kasjer = fork();
    if (pid_kasjer == 0) {
        execl("./kasjer", "kasjer", NULL);
        perror("Nie udało się uruchomić procesu kasjera");
        exit(EXIT_FAILURE);
    }
    pid_pracownik = fork();
    if (pid_pracownik == 0) {
        execl("./pracownik", "pracownik", NULL);
        perror("Nie udało się uruchomić procesu pracownika");
        exit(EXIT_FAILURE);
    }
    printf("Pracownik PID: %d\n", pid_pracownik);
    printf("Kasjer PID: %d\n", pid_kasjer);
    
    for (int i = 0; i < 20; i++) {
        if (fork()) {
                execl("./narciarz", "narciarz", NULL);
                perror("Nie udało się uruchomić procesu narciarza");
                exit(EXIT_FAILURE);
            }
            sleep(1);
    }
    pause();
    return 0;
}

void start_msg() {
    // Utwórz kolejkę komunikatów
    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("Nie udało się utworzyć kolejki komunikatów");
        exit(EXIT_FAILURE);
    }
}
void start_sem() {
    // Utwórz zestaw semaforów
    sem_id = semget(SEM_KEY, TOTAL_SEMS, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("Nie można utworzyć semaforów");
        exit(EXIT_FAILURE);
    }
    if (semctl(sem_id, SEM_GATE1, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_GATE2, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_GATE3, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_GATE4, SETVAL, 1) == -1 ||
        semctl(sem_id, SEM_QUEUE, SETVAL, 3) == -1 ||
        semctl(sem_id, SEM_RIDE_UP, SETVAL, 0) == -1) {
            perror("Nie można zainicjalizować semafora bramki");
            exit(EXIT_FAILURE);
    }

}
void start_shm() {
    // Utwórz segment pamięci współdzielonej
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Nie można utworzyć pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    // Przyłącz segment
    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie można dołączyć do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }
}

void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nZatrzymano działanie stacji. Czyszczenie zasobów...\n");
        shm_ptr->is_running = 0;
        sleep(5);
        clean();
        exit(0);
    }
}

void clean() {
    // Usuń pamięć współdzieloną
    if (shmdt(shm_ptr) == -1) {
        perror("Nie można odłączyć pamięci współdzielonej");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Nie można usunąć pamięci współdzielonej");
    }

    // // Usuń semafory
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Nie można usunąć semaforów");
    }

    // Usuń kolejkę komunikatów
    if (msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("Nie można usunąć kolejki komunikatów");
    }
}