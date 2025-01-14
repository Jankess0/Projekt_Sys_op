#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <pthread.h>
#include "shared.h"

int shm_id;
int sem_id;
int msg_id;
SharedData *shm_ptr;


void start_shmemory();
void wait_sem();
void signal_sem();
void start_msg();
void send_msg(int msg_id, int pid, int total_people);
void* monitor_peron_dolny(void *arg);
int dec_queue(FifoQueue *queue, FifoEntry *entry);


int main() {
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    start_shmemory();
    start_msg();

    printf("Pracownik1 uruchomiony, monitoruje dolna stacje.\n");
    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, monitor_peron_dolny, NULL) != 0) { // watek monitorujacy stacje
        perror("Nie udało się utworzyć wątku monitorującego peron dolny");
        exit(EXIT_FAILURE);
    }
    while (1) {
        wait_sem();
        if (shm_ptr -> lift_status == 0) {
            usleep(1000000);
            continue;
        }

        FifoEntry entry;

        // Pobranie narciarza z kolejki FIFO
        if (dec_queue(&shm_ptr->fifo_queue, &entry) == 0) {
            int total_people = 1 + entry.num_children; // Narciarz + dzieci
            printf("Pracownik: Obsługuję narciarza %d z %d dziećmi (łącznie %d osób).\n",
                   entry.ticket_id, entry.num_children, total_people);

          
            if (shm_ptr->num_chairs + total_people > MAX_CHAIRS * 3) {
                printf("Pracownik: Brak miejsc na krzesełkach dla narciarza %d. Czekam...\n",
                       entry.ticket_id);
                signal_sem();
                sleep(1);
                continue;
            }

            // Umieszczanie narciarza na krzeselkach
            shm_ptr->num_chairs += 1;
            printf("Pracownik: Narciarz %d zajmuje %d miejsca. Zajęte miejsca: %d/%d\n",
                   entry.ticket_id, total_people, shm_ptr->num_chairs, MAX_CHAIRS * 3);
            send_msg(msg_id, entry.pid, total_people);
            shm_ptr->num_people_lower -= total_people;
            signal_sem();

            
        } else {
            //printf("Pracownik: Kolejka jest pusta. Czekam na narciarzy...\n");
        }
        
        signal_sem();
        usleep(500000);
    }
    return 0;
}

int dec_queue(FifoQueue *queue, FifoEntry *entry) {
    if (queue->front == queue->rear) return -1; //kolejka pusta
    *entry = queue->queue[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    return 0;
}

void start_shmemory() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Pracownik1: Nie można utworzyć pamięci współdzielonej");
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

void start_msg() {
    msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msg_id == -1) {
        perror("Nie udalo sie utworzyc kolejki komunikatow");
        exit(EXIT_FAILURE);
    }
}

void send_msg(int msg_id, int pid, int total_people) {
    Message msg;
    msg.msg_type = 1;
    msg.pid = pid;
    msg.total_people = total_people;

    if (msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Nie udalo sie wyslac komunikatu");
    }
    
}

void* monitor_peron_dolny(void *arg) {
    while (1) {
        wait_sem();
        if (shm_ptr->num_chairs >= MAX_CHAIRS) {
            printf("Pracownik1: Przepełnienie peronu dolnego! Zatrzymuję kolejkę.\n");
            shm_ptr->lift_status = 0;
            kill(getppid(), SIGUSR1); // Wysyłanie sygnału zatrzymania do stacji centralnej
        } else if (shm_ptr->lift_status == 0 && shm_ptr->num_people_lower <= MAX_PERSONS) {
            printf("Pracownik1: Warunki spełnione. Wznowienie pracy kolejki.\n");
            shm_ptr->lift_status = 1;
            kill(getppid(), SIGUSR2);
        }
        signal_sem();
        usleep(100000); 
    }
}
